public interface MemSim
{
    /**
     * Prints out the simulation information
     * @return String representation of the simulation
     */
    public String toString();

    /**
     * Simulates one memory access
     * @param accessType Type of access, load or store
     * @param address Memory address to be accessed
     * @param process Process accessing the memory. 1 or 0
     */
    public void simulate(char accessType, long address, int process, int line);
}
