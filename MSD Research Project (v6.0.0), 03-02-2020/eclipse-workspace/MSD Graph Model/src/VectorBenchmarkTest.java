
public class VectorBenchmarkTest {

	public static void main(String[] args) {
		long t = System.nanoTime();
		for (int i = 0; i < 1E7; i++) {
			Vector v = new Vector(1, 1, 1);
		}
		t = System.nanoTime() - t;
		System.out.printf("Time: %.6f ms%n", t / 1E6);
	}

}
