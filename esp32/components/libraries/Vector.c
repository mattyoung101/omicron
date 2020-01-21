#include <Vector.h>

static float calcMag(float x, float y)
{
    return fmod(sqrtf(pow(x, 2) + pow(y, 2)), 360.0f);
}

static float calcArg(float x, float y)
{
    return fmod(450.0f - atan2(y, x) * 180.0f / 3.14159, 360.0f);
}

static float calcX(float mag, float arg)
{
    return mag * (cosf(fmod((450.0f - arg) * 3.14159 / 180.0f, 360.0f)));
}

static float calcY(float mag, float arg)
{
    return mag * (sinf(fmod((450.0f - arg) * 3.14159 / 180.0f, 360.0f)));
}

vect_2d_t vect_2d(float xOrMag, float yOrArg, bool isPolar)
{
    vect_2d_t newVect;
    if (isPolar)
    {
        newVect.mag = fmod(xOrMag, 360.0f);
        newVect.arg = fmod(yOrArg, 360.0f);
        newVect.x = calcX(xOrMag, yOrArg);
        newVect.y = calcY(xOrMag, yOrArg);
    }
    else
    {
        newVect.x = xOrMag;
        newVect.y = yOrArg;
        newVect.mag = fmod(calcMag(xOrMag, yOrArg), 360.0f);
        newVect.arg = fmod(calcArg(xOrMag, yOrArg), 360.0f);
    }
    return newVect;
}

vect_2d_t add_vect_2d(vect_2d_t one, vect_2d_t two)
{
    vect_2d_t newVect;
    newVect.x = one.x + two.x;
    newVect.y = one.y + two.y;
    newVect.arg = fmod(calcArg(newVect.x, newVect.y), 360.0f);
    newVect.mag = fmod(calcMag(newVect.x, newVect.y), 360.0f);
    return newVect;
}

vect_2d_t subtract_vect_2d(vect_2d_t one, vect_2d_t two)
{
    vect_2d_t newVect;
    newVect.x = one.x - two.x;
    newVect.y = one.y - two.y;
    newVect.mag = calcMag(newVect.x, newVect.y);
    newVect.arg = calcArg(newVect.x, newVect.y);
    return newVect;
}

vect_2d_t scalar_multiply_vect_2d(vect_2d_t one, float scalar)
{
    vect_2d_t newVect;
    newVect.mag = one.mag * scalar;
    newVect.arg = one.arg;
    newVect.x = calcX(newVect.mag, newVect.arg);
    newVect.y = calcY(newVect.mag, newVect.arg);
    return newVect;
}