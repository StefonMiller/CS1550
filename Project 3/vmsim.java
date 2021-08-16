import java.io.File;
import java.io.FileNotFoundException;
import java.util.Scanner;

public class vmsim
{
    public static void main(String[] args) throws FileNotFoundException {
        //Algorithm to be used
        String algorithm = null;
        //Number of total frames
        int numFrames = 0;
        //Page size in KB
        int pageSize = 0;
        //Ratio of frames to each process
        String[] memSplit = null;
        int a = 0;
        int b = 0;
        //Trace file variables
        String fName = null;
        File traceFile = null;
        Scanner fReader = null;
        //Number of hex digits used for the page number
        int pageNumberOffset = 0;
        //Current line in the file
        int line = 0;

        //Ensure we have enough arguments for execution
        if(args.length != 9)
        {
            System.out.println("Invalid arguments");
            System.exit(-1);
        }

        //Read in data from args
        try
        {
            algorithm = args[1];
            numFrames = Integer.parseInt(args[3]);
            pageSize = Integer.parseInt(args[5]);
            memSplit = args[7].split(":");
            fName = args[8];
            a = Integer.parseInt(memSplit[0]);
            b = Integer.parseInt(memSplit[1]);
            /*
                Calculate the number of hex digits used for the page offset in the trace file.
                Get Log base 2 of the page size + 10(b/c we are in units of KB)
             */
            pageNumberOffset = (int) Math.ceil((((Math.log(pageSize) / Math.log(2))) + 10));
        }
        catch(NumberFormatException nf)
        {
            //Ensure data is valid before proceeding
            System.out.println("Invalid argument(s)");
            System.exit(-1);
        }

        //Prepare to read data in from the file
        try
        {
            traceFile = new File(fName);
            fReader = new Scanner(traceFile);
        }
        catch (FileNotFoundException e)
        {
            System.out.println("Algorithm: " + algorithm + "\n" +
                "Number of frames: " + numFrames + "\n" +
                "Page size: " + pageSize + " KB\n" +
                "Total memory accesses: " + 0 + "\n" +
                "Total page faults: " + 0 + "\n" +
                "Total writes to disk: " + 0);
            System.exit(0);
        }

        MemSim simulation = null;

        //Simulate the corresponding page replacement algorithm for the trace file
        if(algorithm.equals("opt"))
        {
            //Create OPT simulation object
            simulation = new OPTSim(numFrames, pageSize, a, b, traceFile, pageNumberOffset);
        }
        else if(algorithm.equals("lru"))
        {
            //Create LRU simulation object
            simulation = new LRUSim(numFrames, pageSize, a, b);
        }
        else
        {
            //Ensure either opt or lru are entered for the algorithm
            System.out.println("Invalid algorithm provided");
            System.exit(-1);
        }

        //Once we have the appropriate simulation object, simulate the memory accesses
        //Type of access, load or store
        char accessType = 0;
        //Page number being accessed
        long pageNum = 0;
        //Process accessing memory, either 0 or 1
        int process = 0;
        //Loop through file and simulate the memory access
        while (fReader.hasNextLine())
        {
            String[] data = fReader.nextLine().split(" ");
            try
            {
                accessType = data[0].charAt(0);
                /*
                    Convert only the digits representing the page number to a decimal value to a long.
                    This is calculated by taking only the digits from 0 to the total # - the offset
                 */

                pageNum = Long.decode(data[1]);
                pageNum = pageNum >>> pageNumberOffset;
                process = Integer.parseInt(data[2]);
            }
            catch(NumberFormatException nf)
            {
                System.out.println("Invalid data in trace file");
                nf.printStackTrace();
                System.exit(-1);
            }

            simulation.simulate(accessType, pageNum, process, line);
            line++;

        }
        System.out.println(simulation);

    }

}
