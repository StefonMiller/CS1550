import java.util.LinkedList;

public class LRUSim implements MemSim
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
    //Array of processes in the simulation
    LRUProcess[] processes = new LRUProcess[2];

    public LRUSim(int nf, int ps, int p1, int p2)
    {
        numFrames = nf;
        pageSize = ps;
        //Calculate the number of frames allocated to each process and pass it to the constructor of the LRUProcess class
        processes[0] = new LRUProcess((nf/(p1+p2)) * p1);
        processes[1] = new LRUProcess((nf/(p1+p2)) * p2);
        //Initialize access data
        memAccesses = 0;
        pageFaults = 0;
        diskWrites = 0;
    }

    public void simulate(char accessType, long pageNum, int process, int line)
    {
        //Increment the number of memory accesses for each simulation step
        memAccesses++;
        //Current proceess running
        LRUProcess currProc = processes[process];
        //Check if the page is in RAM
        PageTableEntry currPage = getPage(currProc, pageNum);
        //If the page isn't in RAM, then we need to create and add it
        if(currPage == null)
        {
            pageFaults++;
            //Create PTE
            currPage = new PageTableEntry(pageNum);
            if (accessType == 's')
            {
                currPage.written = true;
            }
            //If there is enough space in RAM, we can just add the process
            if(currProc.loaded < currProc.maxSize)
            {
                currProc.ram.add(currPage);
                currProc.loaded++;
            }
            else
            {
                //If there is not enough space, evict the first entry in the list and add the new entry
                PageTableEntry temp = currProc.ram.remove(0);
                if(temp.written == true)
                {
                    diskWrites++;
                }
                currProc.ram.add(currPage);
            }

        }
        else
        {
            if (accessType == 's')
            {
                currPage.written = true;
            }
        }

    }

    public PageTableEntry getPage(LRUProcess p, long pageNum)
    {
        //Check if there are any pages in RAM
        if(!p.ram.isEmpty())
        {
            //Look for the page in RAM
            for(int i = 0; i < p.ram.size(); i++)
            {
                PageTableEntry currPage = p.ram.get(i);
                if(currPage.pageNum == pageNum)
                {
                    //Add the process to the back of the list when it has been referenced
                    p.ram.add(p.ram.remove(i));
                    return currPage;
                }
            }
        }
        return null;
    }

    public String toString()
    {
        return "Algorithm: LRU\n" +
                "Number of frames: " + numFrames + "\n" +
                "Page size: " + pageSize + " KB\n" +
                "Total memory accesses: " + memAccesses + "\n" +
                "Total page faults: " + pageFaults + "\n" +
                "Total writes to disk: " + diskWrites;
    }

    //Inner class representing a process using LRU
    private class LRUProcess
    {
        //RAM used by process
        long maxSize;
        //Linked list representing the memory allocated to the process
        LinkedList<PageTableEntry> ram;
        //Number of pages loaded in ram by the process
        int loaded;

        private LRUProcess(int f)
        {
            //Set max size to frame size and initialize linked list + loaded variables
            maxSize = f;
            ram = new LinkedList<PageTableEntry>();
            loaded = 0;
        }

    }
}
