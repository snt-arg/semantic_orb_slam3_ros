#include "../stuff/misc.h"
#include "isometry3d_mappings.h"

namespace g2o
{
    namespace internal
    {
        Quaternion normalized(const Quaternion &q)
        {
            Quaternion q2(q);
            normalize(q2);
            return q2;
        }

        Quaternion &normalize(Quaternion &q)
        {
            q.normalize();
            if (q.w() < 0)
            {
                q.coeffs() *= -1;
            }
            return q;
        }

        // // functions to handle the rotation part
        // Vector3 toEuler(const Matrix3 &R)
        // {
        //     Quaternion q(R);
        //     const double &q0 = q.w();
        //     const double &q1 = q.x();
        //     const double &q2 = q.y();
        //     const double &q3 = q.z();
        //     double roll =
        //         std::atan2(2 * (q0 * q1 + q2 * q3), 1 - 2 * (q1 * q1 + q2 * q2));
        //     double pitch = std::asin(2 * (q0 * q2 - q3 * q1));
        //     double yaw = std::atan2(2 * (q0 * q3 + q1 * q2), 1 - 2 * (q2 * q2 + q3 * q3));
        //     return Vector3(roll, pitch, yaw);
        // }

        // Matrix3 fromEuler(const Vector3 &v)
        // {
        //     // UNOPTIMIZED
        //     double roll = v[0];
        //     double pitch = v[1];
        //     double yaw = v[2];
        //     double sy = std::sin(yaw * cst(0.5));
        //     double cy = std::cos(yaw * cst(0.5));
        //     double sp = std::sin(pitch * cst(0.5));
        //     double cp = std::cos(pitch * cst(0.5));
        //     double sr = std::sin(roll * cst(0.5));
        //     double cr = std::cos(roll * cst(0.5));
        //     double w = cr * cp * cy + sr * sp * sy;
        //     double x = sr * cp * cy - cr * sp * sy;
        //     double y = cr * sp * cy + sr * cp * sy;
        //     double z = cr * cp * sy - sr * sp * cy;
        //     return Quaternion(w, x, y, z).toRotationMatrix();
        // }

        Vector3 toCompactQuaternion(const Matrix3 &R)
        {
            Quaternion q(R);
            normalize(q);
            return q.coeffs().head<3>();
        }

        // Matrix3 fromCompactQuaternion(const Vector3 &v)
        // {
        //     double w = 1 - v.squaredNorm();
        //     if (w < 0)
        //         return Matrix3::Identity();
        //     else
        //         w = sqrt(w);
        //     return Quaternion(w, v[0], v[1], v[2]).toRotationMatrix();
        // }

        Vector6 toVectorMQT(const Isometry3 &t)
        {
            Vector6 vec;
            vec.block<3, 1>(3, 0) = toCompactQuaternion(extractRotation(t));
            vec.block<3, 1>(0, 0) = t.translation();
            return vec;
        }

        // Vector6 toVectorET(const Isometry3 &t)
        // {
        //     Vector6 vec;
        //     vec.block<3, 1>(3, 0) = toEuler(extractRotation(t));
        //     vec.block<3, 1>(0, 0) = t.translation();
        //     return vec;
        // }

        Vector7 toVectorQT(const Isometry3 &t)
        {
            Quaternion q(extractRotation(t));
            q.normalize();
            Vector7 v;
            v[3] = q.x();
            v[4] = q.y();
            v[5] = q.z();
            v[6] = q.w();
            v.block<3, 1>(0, 0) = t.translation();
            return v;
        }

        // Isometry3 fromVectorMQT(const Vector6 &v)
        // {
        //     Isometry3 t;
        //     t = fromCompactQuaternion(v.block<3, 1>(3, 0));
        //     t.translation() = v.block<3, 1>(0, 0);
        //     return t;
        // }

        // Isometry3 fromVectorET(const Vector6 &v)
        // {
        //     Isometry3 t;
        //     t = fromEuler(v.block<3, 1>(3, 0));
        //     t.translation() = v.block<3, 1>(0, 0);
        //     return t;
        // }

        Isometry3 fromVectorQT(const Vector7 &v)
        {
            Isometry3 t;
            t = Quaternion(v[6], v[3], v[4], v[5]).toRotationMatrix();
            t.translation() = v.head<3>();
            return t;
        }

        // SE3Quat toSE3Quat(const Isometry3 &t)
        // {
        //     SE3Quat result(t.matrix().topLeftCorner<3, 3>(), t.translation());
        //     return result;
        // }

        // Isometry3 fromSE3Quat(const SE3Quat &t)
        // {
        //     Isometry3 result = (Isometry3)t.rotation();
        //     result.translation() = t.translation();
        //     return result;
        // }

    } // end namespace internal
} // end namespace g2o