#include <utils/madgwick.h>
#include <math.h>
#include <iostream>

Madgwick::Madgwick(double period, double gain):
    _q()
{
    _period = period;
    _beta   = gain;
}

Vect Madgwick::compute_F(Quaternion acc, Quaternion mag, Quaternion B)
{
    Vect F = Vect(6);

    F.set(0, 2.0 * (      _q(1)*_q(3) - _q(0)*_q(2)) - acc(1));
    F.set(1, 2.0 * (      _q(0)*_q(1) + _q(2)*_q(3)) - acc(2));
    F.set(2, 2.0 * (0.5 - _q(1)*_q(1) - _q(2)*_q(2)) - acc(3));
    F.set(3, 2.0 * B(1) * (0.5 - _q(2)*_q(2) - _q(3)*_q(3)) + 2.0f * B(3) * (      _q(1)*_q(3) - _q(0)*_q(2)) - mag(1));
    F.set(4, 2.0 * B(1) * (      _q(1)*_q(2) - _q(0)*_q(3)) + 2.0f * B(3) * (      _q(0)*_q(1) + _q(2)*_q(3)) - mag(2));
    F.set(5, 2.0 * B(1) * (      _q(0)*_q(2) + _q(1)*_q(3)) + 2.0f * B(3) * (0.5 - _q(1)*_q(1) - _q(2)*_q(2)) - mag(3));

    return F;
}

Matrix Madgwick::compute_J(Quaternion B)
{
    Matrix J = Matrix(6, 4);

    J.set(0, 0, -2.0 * _q(2));
    J.set(0, 1,  2.0 * _q(3));
    J.set(0, 2, -2.0 * _q(0));
    J.set(0, 3,  2.0 * _q(1));

    J.set(1, 0,  2.0 * _q(1));
    J.set(1, 1,  2.0 * _q(0));
    J.set(1, 2,  2.0 * _q(3));
    J.set(1, 3,  2.0 * _q(2));

    J.set(2, 0,  0.0) ;
    J.set(2, 1, -4.0 * _q(1));
    J.set(2, 2,  2.0 * _q(3));
    J.set(2, 3,  2.0 * _q(2));

    J.set(3, 0, -2.0 * B(3) * _q(2));
    J.set(3, 1,  2.0 * B(3) * _q(3));
    J.set(3, 2, -4.0 * B(1) * _q(2) - 2.0 * B(3) * _q(0));
    J.set(3, 3, -4.0 * B(1) * _q(3) + 2.0 * B(3) * _q(1));

    J.set(4, 0, -2.0 * B(1) * _q(3) + 2.0 * B(3) * _q(1));
    J.set(4, 1,  2.0 * B(1) * _q(2) + 2.0 * B(3) * _q(0));
    J.set(4, 2,  2.0 * B(1) * _q(1) + 2.0 * B(3) * _q(3));
    J.set(4, 3, -2.0 * B(1) * _q(0) + 2.0 * B(3) * _q(2));

    J.set(5, 0,  2.0 * B(1) * _q(2));
    J.set(5, 1,  2.0 * B(1) * _q(3) - 4.0 * B(3) * _q(1));
    J.set(5, 2,  2.0 * B(1) * _q(0) - 4.0 * B(3) * _q(2));
    J.set(5, 3,  2.0 * B(1) * _q(1));

    return J;
}

void Madgwick::update(double gx, double gy, double gz, double ax, double ay, double az, double mx, double my, double mz) 
{
    Quaternion gyr = Quaternion(0.0, gx, gy, gz);
    Quaternion acc = Quaternion(0.0, ax, ay, az);
    Quaternion mag = Quaternion(0.0, mx, my, mz);

    // Compute feedback only if accelerometer measurement valid (avoids NaN in accelerometer normalisation)
    if(acc.norm() != 0.0 && mag.norm() != 0.0) 
    {
        acc.normalize();
        mag.normalize();

        /* Reference direction of Earth's magnetic field */
        Quaternion H    = _q * (mag * _q.conjugate());
        Quaternion B    = Quaternion(0.0, sqrtf(H(1) * H(1) + H(2) * H(2)), 0.0, H(3));

        /* Gradient decent algorithm corrective step */
        Vect F          = compute_F(acc, mag, B);
        Matrix J        = compute_J(B);
        Quaternion step = Quaternion(J.transpose() * F);
        step.normalize();

        /* AHRS Quaternion iteration (integration + correction) */
        _q = _q + (((_q * gyr) * 0.5) - (step * _beta)) * _period;
        _q.normalize();
    }
}

double Madgwick::roll(void)
{
    return _q.roll();
}

double Madgwick::pitch(void)
{
    return _q.pitch();
}

double Madgwick::yaw(void)
{
    return _q.yaw();
}

void Madgwick::rotate(double x, double y, double z, double * x_r, double * y_r, double * z_r)
{
    Quaternion v = Quaternion(4);

    v.set(0.0, x, y, z);

    Quaternion v_r = _q * v * _q.conjugate();

    *x_r = v_r(1);
    *y_r = v_r(2);
    *z_r = v_r(3);
}

Madgwick::~Madgwick()
{
    return;
}
