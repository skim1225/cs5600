/*
 * InsertStrings.java / Synchronization with Monitors
 *
 * Sooji Kim / CS5600 / Northeastern University
 * Fall 2025 / Oct 15, 2025
 *
 * Program for inserting 30+ strings into bounded buffer implementation using 3+ concurrent threads.
 */

public class InsertStrings {

    // class vars
    private static final int CAPACITY = 10;
    private static final int NUM_THREADS = 3;
    private static final int NUM_STRINGS = 30;

    private static int produced = 0;

    /**
     * @brief Entry point of the program.
     *
     * Initializes the bounded buffer, starts multiple producer threads to insert
     * strings, and a single consumer thread to retrieve them. Joins all producer
     * threads and interrupts the consumer upon completion.
     *
     * @param args command-line arguments (not used)
     * @throws InterruptedException if any thread is interrupted during join
     */
    public static void main(String[] args) throws InterruptedException {
        BoundedBuffer buffer = new BoundedBuffer(CAPACITY);

        // create and start getter from buffer
        Thread consumer = new Thread(() -> consume(buffer));
        consumer.start();

        // create and start writers to buffer
        Thread[] producers = new Thread[NUM_THREADS];
        for (int i = 0; i < NUM_THREADS; i++) {
            producers[i] = new Thread(() -> produce(buffer));
            producers[i].start();
        }

        // join threads
        for (Thread t : producers) {
            t.join();
        }
        consumer.interrupt();
    }

    /**
     * @brief Produces and deposits strings into the buffer.
     *
     * Each thread repeatedly generates a unique string identifier composed of its
     * thread ID and a random 5-digit number, then deposits it into the bounded
     * buffer. The loop terminates when the total number of strings reaches the
     * predefined limit.
     *
     * @param buffer the shared {@link BoundedBuffer} instance to deposit items into
     */
    private static void produce(BoundedBuffer buffer) {
        try {
            while (true) {
                int numInserted = numInserted();
                if (numInserted == -1) {
                    break;
                }
                int threadId = (int) (Thread.currentThread().getId());
                String item = threadId + "-" + genRand();
                buffer.deposit(item);
                System.out.println(item + " has been added to the buffer");
            }
        } catch (InterruptedException e) {
            Thread.currentThread().interrupt();
        }
    }

    /**
     * @brief Continuously consumes strings from the buffer.
     *
     * Fetches strings deposited by producer threads and prints them. The loop runs
     * indefinitely until the thread is interrupted.
     *
     * @param buffer the shared {@link BoundedBuffer} instance to fetch items from
     */
    private static void consume(BoundedBuffer buffer) {
        try {
            while (true) {
                String s = buffer.fetch();
                System.out.println(s + " fetched from the buffer");
            }
        } catch (InterruptedException e) {
            return;
        }
    }

    /**
     * @brief Generates a random 5-digit string.
     *
     * Creates a random integer in the range [00000, 99999] and formats it as a
     * zero-padded 5-character string.
     *
     * @return a 5-digit random string
     */
    private static String genRand() {
        int num = (int) (Math.random() * 100000);
        return String.format("%05d", num);
    }

    /**
     * @brief Tracks the total number of strings inserted in a thread-safe manner.
     *
     * Increments the shared counter if the total number of produced strings is less
     * than the predefined limit. Once the limit is reached, returns -1 to signal
     * that production should stop.
     *
     * @return the updated count of inserted strings, or -1 if the limit is reached
     */
    private static synchronized int numInserted() {
        if (produced >= NUM_STRINGS) {
            return -1;
        }
        return produced++;
    }
}