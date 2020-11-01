
public class VectorBenchmarkTest {

	public static void main(String[] args) {
		long t = System.nanoTime();
		for (long i = 0; i < 1E8; i++) {
			Vector v = new Vector();
		}
		t = System.nanoTime() - t;
		System.out.printf("Time: %.6f ms%n", t / 1E6);
	}

}
