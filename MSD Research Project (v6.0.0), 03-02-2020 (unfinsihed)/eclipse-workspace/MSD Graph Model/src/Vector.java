
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
		this(x, y, 0);
	}
	
	public Vector() {
		this(0, 0, 0);
	}
	
	public static final Vector ZERO = new Vector(0, 0, 0);
	public static final Vector I = new Vector(1, 0, 0);
	public static final Vector J = new Vector(0, 1, 0);
	public static final Vector K = new Vector(0, 0, 1);
	
	public static Vector cylindricalForm(double r, double theta, double z) {
		return new Vector( r * Math.cos(theta),  r * Math.sin(theta), z );
	}
	
	public static Vector polarForm(double r, double theta) {
		return cylindricalForm(r, theta, 0);
	}
	
	public static Vector sphericalForm(double rho, double theta, double phi) {
		return cylindricalForm( rho * Math.cos(phi), theta, rho * Math.sin(phi) );
	}
	
	/**
	 * Rotate the angle PI radians; stays in <code>(-PI, PI]</code>
	 * @param angle  An angle in radians between <code>(-2*PI, 2*PI]</code>
	 * @return rotated angle  The given angle rotated by PI radians
	 */
	private static double turn(double angle) {
		return angle + (angle <= 0 ? Math.PI : -Math.PI);
	}
	
	/**
	 * This is a more expensive operation then {@link #normSq()}.
	 * @return The Euclidean norm of the vector
	 */
	public double norm() {
		return Math.sqrt( normSq() );
	}
	
	/**
	 * This is a faster operation then computing the actual {@link #norm()}.
	 * @return the square of the Euclidean norm of the vector
	 */
	public double normSq() {
		return x * x + y * y + z * z;
	}
	
	/**
	 * @return the angle/rotation of the vector when projected into the xy-plane
	 */
	public double theta() {
		return Math.atan2(y, x);
	}
	
	/**
	 * Edge cases: <ul>
	 * 	<li> if the vector is parallel to the z-axis, either <code>Math.PI</code> or <code>-Math.PI</code> is returned. </li>
	 * 	<li> the zero vector returns a phi of <code>0</code> </li>
	 * </ul>
	 * @return the angle of elevation relative to the xy-plane. Phi is always between <code>[-Math.PI, Math.PI]</code>
	 */
	public double phi() {
		double r = Math.sqrt( x * x + y * y );
		if( r == 0 ) {
			if( z > 0 )
				return Math.PI / 2;
			else if( z < 0 )
				return -Math.PI / 2;
			else
				return 0;
		} else
			return Math.atan( z / r );
	}
	
	public boolean equals(Object obj) {
		if (!(obj instanceof Vector))
			return false;
		Vector v = (Vector) obj;
		return x == v.x && y == v.y && z == v.z;
	}
	
	/**
	 * Adds two vectors and returns the resultant vector (the sum).
	 * @param v
	 * @return  <code>this + v</code>
	 */
	public Vector add(Vector v) {
		return new Vector(x + v.x, y + v.y, z + v.z);
	}
	
	/**
	 * Negates a vector and returns the vector pointing in the opposite direction, with the same magnitude.
	 * @return <code>-this</code>
	 */
	public Vector negate() {
		return new Vector(-x, -y, -z);
	}
	
	/**
	 * Subtracts two vectors and returns the difference.
	 * @param v
	 * @return  <code>this - v</code>
	 */
	public Vector subtract(Vector v) {
		return new Vector(x - v.x, y - v.y, z - v.z);
	}
	
	/**
	 * Multiplies a vector by the given scaler and returns the product.
	 * @param k  A scaler
	 * @return <code>k * this</code>
	 */
	public Vector multiply(double k) {
		return new Vector(k * x, k * y, k * z);
	}
	
	/**
	 * Mutiplies teo vectors and returns the inner/dot product.
	 * @param v  A vector
	 * @return <code>this * v</code> (double)
	 */
	public double multiply(Vector v) {
		return x * v.x + y * v.y + z * v.z;
	}
	
	/**
	 * @see #multiply(Vector)
	 */
	public double dotProduct(Vector v) {
		return multiply(v);
	}
	
	/**
	 * Note: this is a slower operation then {@link #distaceSq(Vector)}.
	 * @param v
	 * @return The Euclidean distance between the two vectors.
	 */
	public double distance(Vector v) {
		return Math.sqrt( distanceSq(v) );
	}
	
	/**
	 * Note: this is a faster operation the {@link #distace(Vector)}.
	 * @param v
	 * @return The square of the Euclidean distance between the two vectors.
	 */
	public double distaceSq(Vector v) {
		double dx = v.x - x;
		double dy = v.y - y;
		double dz = v.z - z;
		return dx * dx + dy * dy + dz * dz;
	}
	
	/**
	 * @param v
	 * @return The (smaller) angle between the two vectors.
	 * 		Answer is always between <code>[0, Math.PI]</code>
	 */
	public double angleBetween(Vector v) {
		return Math.acos( dotProduct(v) / Math.sqrt(normSq() * v.normSq()) );
	}
	
	/**
	 * @param v
	 * @return the 3D cross product between the two vectors.
	 */
	public Vector crossProduct(Vector v) {
		return new Vector( y * v.z - z * v.y, z * v.x - x * v.z, x * v.y - y * v.x );
	}
	
	/**
	 * Add <code>v</code> to <code>this</code> modifying the value of <code>this</code>, and return the sum.
	 * @param v
	 * @return <code>this += v</code>
	 */
	public Vector addBy(Vector v) {
		x += v.x;
		y += v.y;
		z += v.z;
		return this;
	}
}
