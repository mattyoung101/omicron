// Credit to Robert McArthur, Team Apex 2019
// who originally wrote this.
#include "Vector.h"

Vector::Vector() {
	setStandard(0, 0);
}

Vector::Vector(double val1, double val2, bool polar){
	polar ? setPolar(val1, val2) : setStandard(val1, val2);
}

void Vector::setStandard(double _i, double _j){
	i = _i;
	j = _j;

	mag = calcMag(_i, _j);
	arg = calcArg(_i, _j);
	if(sign(mag) == -1) {
		arg = mod(arg + 180, 360);
		mag = mag * -1;
	}
}

void Vector::setPolar(double _mag, double _arg){
	i = calcI(_mag, _arg);
	j = calcJ(_mag, _arg);

	mag = _mag;
	arg = doubleMod(_arg, 360);
}

Vector Vector::operator+(Vector vector2){
	return Vector(i + vector2.i, j + vector2.j, false);
}

Vector Vector::operator-(Vector vector2){
	return Vector(i - vector2.i, j - vector2.j, false);
}

Vector Vector::operator*(double scalar){
	return Vector(mag * scalar, arg);
}

Vector Vector::operator/(double scalar){
	return Vector(mag / scalar, arg);
}

void Vector::operator+=(Vector vector2){
	setStandard(i + vector2.i, j + vector2.j);
}

void Vector::operator-=(Vector vector2){
	setStandard(i - vector2.i, j - vector2.j);
}

Vector Vector::operator*=(double scalar){
	return Vector(mag * scalar, arg);
}

Vector Vector::operator/=(double scalar){
	return Vector(mag / scalar, arg);
}

bool Vector::operator==(Vector vector2){
	return mag == vector2.mag;
}

bool Vector::operator!=(Vector vector2){
	return mag != vector2.mag;
}

bool Vector::operator<(Vector vector2){
	return mag < vector2.mag;
}

bool Vector::operator<=(Vector vector2){
	return mag <= vector2.mag;
}

bool Vector::operator>(Vector vector2){
	return mag > vector2.mag;
}

bool Vector::operator>=(Vector vector2){
	return mag >= vector2.mag;
}

bool Vector::exists(){
	return mag != 0;
}

bool Vector::isBetween(double leftAngle, double rightAngle){
	return angleIsInside(arg, leftAngle, rightAngle);
}

double Vector::calcI(double _mag, double _arg){
	return _mag * sin(degreesToRadians(_arg));
}

double Vector::calcJ(double _mag, double _arg){
	return _mag * cos(degreesToRadians(_arg));
}

double Vector::calcMag(double _i, double _j){
	return sqrt(_i * _i + _j * _j);
}

double Vector::calcArg(double _i, double _j){
	return doubleMod(450 - radiansToDegrees(atan2(_j, _i)),360);
}
