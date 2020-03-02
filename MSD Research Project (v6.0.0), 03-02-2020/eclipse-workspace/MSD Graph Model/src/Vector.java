
public class Vector {
	public double x;
	public double y;
	public double z;
	
	public Vector(double x, double y, double z) {
		this.x = x;
		this.y = y;
		this.z = z;
	}
	
	public Vector(double x, double y) {
		this.x = x;
		this.y = y;
	}
	
	public Vector() {
	}
	
	public static final Vector ZERO = new Vector();
	public static final Vector I = new Vector(1, 0, 0);
	public static final Vector J = new Vector(0, 1, 0);
	public static final Vector K = new Vector(0, 0, 1);
	
	public static Vector cylindricalForm(double r, double theta, double z) {
		// TODO
		return null;
	}
	
	public static Vector polarForm(double r, double theta) {
		// TODO
		return null;
	}
	
	public static Vector sphericalForm(double rho, double theta, double phi) {
		// TODO
		return null;
	}
	
	/**
	 * Rotate the angle PI radians; stays in [PI, -PI)
	 * @param angle
	 * @return rotated angle
	 */
	private double turn(double angle) {
		// TODO
		return Double.NaN;
	}
	
	
}
