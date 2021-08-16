import java.io.File;
import java.io.FileNotFoundException;
import java.util.*;

public class OPTSim implements MemSim
{
    //Size of each page
    int pageSize;
    //Number of total frames
    int numFrames;
    //Number of memory accesses total
    int memAccesses;
    //Number of page faults
    int pageFaults;
    //Number of diskWrites
    int diskWrites;

    OPTProcess[] processes = new OPTProcess[2];

    @SuppressWarnings("unchecked")
    public OPTSim(int nf, int ps, int p1, int p2, File f, int pno) throws FileNotFoundException
    {
        numFrames = nf;
        pageSize = ps;

        Scanner fReader = new Scanner(f);
        HashMap<Long, PageTableEntry>[] processMemory = new HashMap[2];
        processMemory[0] = new HashMap<>();
        processMemory[1] = new HashMap<>();
        //Populate hash map with an initial pass of the trace file
        int line = 0;
        while(fReader.hasNextLine())
        {
            //Get next line of file and parse in the address, retrieving only the page number
            String[] data = fReader.nextLine().split(" ");
            int process = Integer.parseInt(data[2]);
            //Get current process accessing the address
            HashMap<Long, PageTableEntry> temp = processMemory[process];
            long pageNum = Long.decode(data[1]);
            pageNum = pageNum >> pno;
            //If the page # doesn't exist as a key in the hashmap, then we need to make a new entry
            PageTableEntry page = temp.get(pageNum);
            if(page == null)
            {
                //Create a new page and add the current line to the PTE, then add the page to the pagetable
                page = new PageTableEntry(line, pageNum);
                page.accesses.add(line);
                temp.put(pageNum, page);
            }
            else
            {
                //Add the current line to the corresponding PTE's linked list
                page.accesses.add(line);
            }
            line++;
        }
        //Calculate the number of frames allocated to each process and pass it to the constructor of the OPTProcess class
        processes[0] = new OPTProcess(((nf/(p1+p2)) * p1), processMemory[0]);
        processes[1] = new OPTProcess(((nf/(p1+p2)) * p2), processMemory[1]);
        //Initialize access data
        memAccesses = 0;
        pageFaults = 0;
        diskWrites = 0;

    }

    public void simulate(char accessType, long pageNum, int process, int line)
    {
        //Always increment memAccesses
        memAccesses++;
        //Get the current process
        OPTProcess currProc = processes[process];
        //Get the reference of the PTE with the corresponding page number
        PageTableEntry currPage = currProc.pageTable.get(pageNum);
        currPage.accesses.remove(0);
        //Set the dirty bit if needed
        if(accessType == 's')
        {
            currPage.written = true;
        }
        //Set the last access of the page to the current line
        currPage.lastAccess = line;
        //If the current process is already in RAM, update the entry
        if(!currProc.ram.contains(pageNum))
        {
            //If the page isn't in RAM, there is a page fault
            pageFaults++;
            //If the page is not in RAM, check if there is space to add it
            if(currProc.ram.size() == currProc.maxSize)
            {
                //If there is no space in RAM for the page, select a page to evict
                Long toRemove = getOptimalPage(currProc);
                //Once we have a page to evict, remove that page from RAM
                currProc.ram.remove(toRemove);
                //Check that processes dirty bit to see if there is a disk write involved
                if(currProc.pageTable.get(toRemove).written)
                {
                    diskWrites++;
                    currProc.pageTable.get(toRemove).written = false;
                }

            }
            //Add the page regardless to RAM regardless
            currProc.ram.add(currPage.pageNum);
        }

    }

    public Long getOptimalPage(OPTProcess o)
    {
        //ArrayListof pages with no future accesses
        ArrayList<PageTableEntry> noAccesses = new ArrayList<>();
        //Current best page to remove
        int nextAccess = Integer.MIN_VALUE;
        Long bestPage = null;
        for(Long l : o.ram)
        {
            PageTableEntry value = o.pageTable.get(l);

            //If there are no future accesses, add the page to the bestPages array
            if(value.accesses.isEmpty())
            {
                noAccesses.add(value);
            }
            else
            {
                //If the current page has an access after the current best page to remove, set the new page to the best one to remove
                if(value.accesses.get(0) > nextAccess)
                {
                    //Set bestPage to the current page and the nextAccess to this pages next access line
                    bestPage = value.pageNum;
                    nextAccess = value.accesses.get(0);
                }

            }
        }
        //Check if there are pages with no future accesses
        if(!noAccesses.isEmpty())
        {
            //Get the process with no future accesses that was least recently used
            int lastAccess = Integer.MAX_VALUE;
            for (PageTableEntry noAccessPage : noAccesses)
            {
                if (noAccessPage.lastAccess < lastAccess)
                {
                    bestPage = noAccessPage.pageNum;
                    lastAccess = noAccessPage.lastAccess;
                }
            }
        }
        return bestPage;
    }

    public String toString()
    {
        return "Algorithm: OPT\n" +
                "Number of frames: " + numFrames + "\n" +
                "Page size: " + pageSize + " KB\n" +
                "Total memory accesses: " + memAccesses + "\n" +
                "Total page faults: " + pageFaults + "\n" +
                "Total writes to disk: " + diskWrites;
    }

    //Inner class representing a process using OPT
    private class OPTProcess
    {
        //HashSet of integers representing the current pages in RAM
        HashSet<Long> ram;
        //HashMap of the line accesses per page
        HashMap<Long, PageTableEntry> pageTable;
        int maxSize;

        private OPTProcess(int f, HashMap<Long, PageTableEntry> h)
        {
            //Initialize collections and variables
            ram = new HashSet<>();
            pageTable = h;
            maxSize = f;
        }

    }

}
