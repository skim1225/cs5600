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

    // method for adding to buffer
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

    // method for getting from buffer
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

    // helper func to generate random 5 digit numbers
    private static String genRand() {
        int num = (int) (Math.random() * 100000);
        return String.format("%05d", num);
    }

    // helper func to keep track of num strings inserted safely
    private static synchronized int numInserted() {
        if (produced >= NUM_STRINGS) {
            return -1;
        }
        return produced++;
    }
}