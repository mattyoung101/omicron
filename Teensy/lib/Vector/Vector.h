#ifndef VECTOR_H_
#define VECTOR_H_

#include <Utils.h>

class Vector{
	public:
		Vector();
		// Sets up vector in either polar or standard form
		Vector(double val1, double val2, bool isPolar = true);

		// Sets a vector in standard form
		void setStandard(double _i, double _j);
		// Convert to standard and set i and j
		void setPolar(double _mag, double _arg);

		// Vector addition
		Vector operator+(Vector vector2);
		// Vector subtraction
		Vector operator-(Vector vector2);
		// Scalar multiplication
		Vector operator*(double scalar);
		// Scalar division
		Vector operator/(double scalar);
		// Vector addition
		void operator+=(Vector vector2);
		// Vector subtraction
		void operator-=(Vector vector2);
		// Scalar multiplication
		Vector operator*=(double scalar);
		// Scalar division
		Vector operator/=(double scalar);

		// Checks whether vectors are equivalent in magnitude
		bool operator==(Vector vector2);
		// Checks whether vectors are not equivalent in magnitude
		bool operator!=(Vector vector2);
		// Checks whether the magnitude is less than the second vector
		bool operator<(Vector vector2);
		// Checks whether the magnitude is less than or equal to the second vector
		bool operator<=(Vector vector2);
		// Checks whether the magnitude is greater than the second vector
		bool operator>(Vector vector2);
		// Checks whether the magnitude is greater than or equal to the second vector
		bool operator>=(Vector vector2);

		// Checks whether the vector is the zero vector
		bool exists();
		// Checks whether the vector's argument is between two angles
		bool isBetween(double leftAngle, double rightAngle);

		double i, j, mag, arg;
		
	private:
		// Calculates the i component of a vector
		double calcI(double _mag, double _arg);
		// Calculates the j component of a vector
		double calcJ(double _mag, double _arg);

		// Calculates the magnitude of a vector
		double calcMag(double _i, double _j);
		// Calculates the argument of a vector
		double calcArg(double _i, double _j);
};

#endif