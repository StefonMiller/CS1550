import java.util.LinkedList;

public class PageTableEntry
{
    //Int referring to the last line that accessed the page
    int lastAccess;
    //Long indicating the page's number
    long pageNum;
    //Flag indicating if the page has been written to
    boolean written;
    //List of memory accesses to the page, only needed for OPT
    LinkedList<Integer> accesses;

    /**
     * Constructor for OPT
     * @param l
     * @param pn
     */
    public PageTableEntry(int l, long pn)
    {
        lastAccess = l;
        pageNum = pn;
        written = false;
        accesses = new LinkedList<Integer>();
    }

    /**
     * Constructor for LRU
     * @param pn
     */
    public PageTableEntry(long pn)
    {
        pageNum = pn;
        written = false;
    }


    public String toString()
    {
        return String.valueOf(pageNum);
    }



}
