/**
 * This file is part of ORB-SLAM3
 *
 * Copyright (C) 2017-2021 Carlos Campos, Richard Elvira, Juan J. Gómez Rodríguez, José M.M. Montiel and Juan D. Tardós, University of Zaragoza.
 * Copyright (C) 2014-2016 Raúl Mur-Artal, José M.M. Montiel and Juan D. Tardós, University of Zaragoza.
 *
 * ORB-SLAM3 is free software: you can redistribute it and/or modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * ORB-SLAM3 is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even
 * the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with ORB-SLAM3.
 * If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef TRACKING_H
#define TRACKING_H

#include <opencv2/core/core.hpp>
#include <opencv2/features2d/features2d.hpp>

#include "Viewer.h"
#include "FrameDrawer.h"
#include "Atlas.h"
#include "LocalMapping.h"
#include "LoopClosing.h"
#include "Frame.h"
#include "ORBVocabulary.h"
#include "KeyFrameDatabase.h"
#include "ORBextractor.h"
#include "MapDrawer.h"
#include "System.h"
#include "ImuTypes.h"
#include "Settings.h"
#include "Semantic/Wall.h"
#include "Semantic/Door.h"
#include "Semantic/Room.h"
#include "Semantic/Marker.h"
#include "GeometricCamera.h"

#include <pcl/point_cloud.h>
#include <pcl/point_types.h>
#include <pcl/segmentation/sac_segmentation.h>
#include <pcl/filters/extract_indices.h>

#include <mutex>
#include <unordered_set>

namespace ORB_SLAM3
{

    class Viewer;
    class FrameDrawer;
    class Atlas;
    class LocalMapping;
    class LoopClosing;
    class System;
    class Settings;

    class Tracking
    {

    public:
        EIGEN_MAKE_ALIGNED_OPERATOR_NEW
        Tracking(System *pSys, ORBVocabulary *pVoc, FrameDrawer *pFrameDrawer, MapDrawer *pMapDrawer, Atlas *pAtlas,
                 KeyFrameDatabase *pKFDB, const string &strSettingPath, const int sensor, Settings *settings, const string &_nameSeq = std::string());

        ~Tracking();

        // Parse the config file
        bool ParseCamParamFile(cv::FileStorage &fSettings);
        bool ParseORBParamFile(cv::FileStorage &fSettings);
        bool ParseIMUParamFile(cv::FileStorage &fSettings);

        // Preprocess the input and call Track(). Extract features and performs stereo matching.
        Sophus::SE3f GrabImageStereo(const cv::Mat &imRectLeft, const cv::Mat &imRectRight,
                                     const double &timestamp, string filename);
        Sophus::SE3f GrabImageRGBD(const cv::Mat &imRGB, const cv::Mat &imD, const double &timestamp,
                                   string filename, const std::vector<Marker *> markers,
                                   const std::vector<Door *> doors, const std::vector<Room *> rooms);
        Sophus::SE3f GrabImageMonocular(const cv::Mat &im, const double &timestamp, string filename);

        void GrabImuData(const IMU::Point &imuMeasurement);

        void SetLocalMapper(LocalMapping *pLocalMapper);
        void SetLoopClosing(LoopClosing *pLoopClosing);
        void SetViewer(Viewer *pViewer);
        void SetStepByStep(bool bSet);
        bool GetStepByStep();

        // Load new settings
        // The focal lenght should be similar or scale prediction will fail when projecting points
        void ChangeCalibration(const string &strSettingPath);

        // Use this function if you have deactivated local mapping and you only want to localize the camera.
        void InformOnlyTracking(const bool &flag);

        void UpdateFrameIMU(const float s, const IMU::Bias &b, KeyFrame *pCurrentKeyFrame);
        KeyFrame *GetLastKeyFrame()
        {
            return mpLastKeyFrame;
        }

        Sophus::SE3f GetCamTwc();
        Sophus::SE3f GetImuTwb();
        Eigen::Vector3f GetImuVwb();
        bool isImuPreintegrated();

        void CreateMapInAtlas();
        // std::mutex mMutexTracks;

        //--
        void NewDataset();
        int GetNumberDataset();
        int GetMatchesInliers();

        // DEBUG
        void SaveSubTrajectory(string strNameFile_frames, string strNameFile_kf, string strFolder = "");
        void SaveSubTrajectory(string strNameFile_frames, string strNameFile_kf, Map *pMap);

        float GetImageScale();

        // Semantic Entities
        /**
         * @brief Corrects the given plane equations to apply calculations
         * @param plane the input plane
         */
        Eigen::Vector4d correctPlaneDirection(const Eigen::Vector4d &plane);

        /**
         * @brief Gets the center points of a room with two walls
         * @param markerPosition the position of the marker
         * @param wall1 the first plane
         * @param wall2 the second plane
         */
        Eigen::Vector3d getRoomCenter(const Eigen::Vector3d &markerPosition,
                                      const Eigen::Vector4d &wall1,
                                      const Eigen::Vector4d &wall2);

        /**
         * @brief Gets the center points of a room with four walls
         * @param x_plane1 the first plane in X direction
         * @param x_plane2 the second plane in X direction
         * @param y_plane1 the first plane in Y direction
         * @param y_plane2 the second plane in Y direction
         */
        Eigen::Vector3d getRoomCenter(const Eigen::Vector4d x_plane1, const Eigen::Vector4d x_plane2,
                                      const Eigen::Vector4d y_plane1, const Eigen::Vector4d y_plane2);

        /**
         * @brief Associates a detected wall into the walls found in the map and returns
         * if it needs to be added to the plane or not.
         * @param mappedWalls an array of walls with their IDs and equations
         * @param givenPlane the detected 3D plane
         */
        int associateWalls(const vector<Wall *> &mappedWalls, g2o::Plane3D givenPlane);

        /**
         * @brief Calculation of the equation of the plane from marker pose
         * @param rotationMatrix the rotation matrix
         * @param translation the translation matrix
         */
        Eigen::Vector4d getPlaneEquationFromPose(const Eigen::Matrix3f &rotationMatrix,
                                                 const Eigen::Vector3f &translation);

        /**
         * @brief Calculation of plane equation from map points
         * 
        */
        bool getPlaneEquationFromPoints(const Marker* currentMarker, Eigen::Vector4d& planeEstimate);

        /**
         * @brief Get the points close to a given location
         * 
         * @param points 
         * @param location 
         * @param distanceThreshold  
         */
        std::vector<MapPoint*> findPointsCloseToLocation(const std::vector<MapPoint*>& points,
                                                               const Eigen::Vector3f& location,
                                                               double distanceThreshold);

        /**
         * @brief Calculate L2 norm between points
         * 
         * @param point1 
         * @param point2 
         *
         */
        double calculateDistance(const Eigen::Vector3f& p1, const Eigen::Vector3f& p2);    

        /**
         * @brief Perform PCL ransac to get the plane equation from the points
         * 
         * @param points 
         */
        Eigen::Vector4d ransacPlaneFitting(const std::vector<MapPoint*>& points);    

        /**
         * @brief Checks to see if the marker is attached to a wall or not (e.g., a door)
         * and returns the name of it if exists (only valid for doors)
         * @param markerId the id of the marker
         */
        std::pair<bool, std::string> markerIsPlacedOnWall(const int &markerId);

        /**
         * @brief Finds the point lying on wall
         * @param planeEquation equation of a given plane
         * @param mapPoint current map point
         */
        bool pointOnPlane(Eigen::Vector4d planeEquation, MapPoint *mapPoint);

        /**
         * @brief Creates a new marker object to be added to the map
         * @param visitedMarker the address of the visited marker
         * @param pKF the address of the current keyframe
         */
        Marker *createMapMarker(const Marker *visitedMarker, KeyFrame *pKF);

        /**
         * @brief Creates a new wall object to be added to the map
         * @param attachedMarker the address of the attached marker
         * @param estimatedPlane the equation of the plane estimated from the marker
         * @param mapPoints all the map points to check the ones lying on the wall
         * @param pKF the address of the current keyframe
         */
        void createMapWall(Marker *attachedMarker, const g2o::Plane3D estimatedPlane, KeyFrame *pKF);

        /**
         * @brief Updates an existing wall object in the map
         * @param wallId the identifier of the existing wall
         * @param visitedMarker the address of the visited marker
         * @param pKF the address of the current keyframe
         */
        void updateMapWall(int wallId, Marker *visitedMarker, ORB_SLAM3::KeyFrame *pKF);

        /**
         * @brief Creates a new door object to be added to the map
         * @param attachedMarker the address of the attached marker
         * @param pKF the address of the current keyframe
         * @param name the name of the door
         */
        void createMapDoor(Marker *attachedMarker, KeyFrame *pKF, std::string name);

        /**
         * @brief Creates a new room object (corridor or room) to be added to the map
         * @param detectedRoom the address of the detected room
         * @param markerIds the list of the detected marker-ids belong to the wall
         */
        void createMapRoom(Room *detectedRoom, std::vector<int> markerIds);

        void reorganizeRoomWalls(ORB_SLAM3::Room *detectedRoom);

        /**
         * @brief Early creation of a room as soon as all elements of at least one of its pairs has been seen
         * (e.g., if all elements of [[1,2,3,4]] or both 1 & 2 in [[1,2][3,4]] visited)
         */
        void earlyRoomDetection();

#ifdef REGISTER_LOOP
        void RequestStop();
        bool isStopped();
        void Release();
        bool stopRequested();
#endif

    public:
        // Tracking states
        enum eTrackingState
        {
            SYSTEM_NOT_READY = -1,
            NO_IMAGES_YET = 0,
            NOT_INITIALIZED = 1,
            OK = 2,
            RECENTLY_LOST = 3,
            LOST = 4,
            OK_KLT = 5
        };

        eTrackingState mState;
        eTrackingState mLastProcessedState;

        // Input sensor
        int mSensor;

        // Current Frame
        Frame mCurrentFrame;
        Frame mLastFrame;

        cv::Mat mImGray;

        // Initialization Variables (Monocular)
        std::vector<int> mvIniLastMatches;
        std::vector<int> mvIniMatches;
        std::vector<cv::Point2f> mvbPrevMatched;
        std::vector<cv::Point3f> mvIniP3D;
        Frame mInitialFrame;
        Sophus::SE3f Tc0w;

        // Lists used to recover the full camera trajectory at the end of the execution.
        // Basically we store the reference keyframe for each frame and its relative transformation
        list<Sophus::SE3f> mlRelativeFramePoses;
        list<KeyFrame *> mlpReferences;
        list<double> mlFrameTimes;
        list<bool> mlbLost;

        // frames with estimated pose
        int mTrackedFr;
        bool mbStep;

        // True if local mapping is deactivated and we are performing only localization
        bool mbOnlyTracking;

        void Reset(bool bLocMap = false);
        void ResetActiveMap(bool bLocMap = false);

        float mMeanTrack;
        bool mbInitWith3KFs;
        double t0;    // time-stamp of first read frame
        double t0vis; // time-stamp of first inserted keyframe
        double t0IMU; // time-stamp of IMU initialization
        bool mFastInit = false;

        vector<MapPoint *> GetLocalMapMPS();

        bool mbWriteStats;

        // Semantic map entities
        std::vector<ORB_SLAM3::Room *> env_rooms;
        std::vector<ORB_SLAM3::Door *> env_doors;

#ifdef REGISTER_TIMES
        void LocalMapStats2File();
        void TrackStats2File();
        void PrintTimeStats();

        vector<double> vdRectStereo_ms;
        vector<double> vdResizeImage_ms;
        vector<double> vdORBExtract_ms;
        vector<double> vdStereoMatch_ms;
        vector<double> vdIMUInteg_ms;
        vector<double> vdPosePred_ms;
        vector<double> vdLMTrack_ms;
        vector<double> vdNewKF_ms;
        vector<double> vdTrackTotal_ms;
#endif

    protected:
        // Main tracking function. It is independent of the input sensor.
        void Track();

        // Map initialization for stereo and RGB-D
        void StereoInitialization();

        // Map initialization for monocular
        void MonocularInitialization();

        // void CreateNewMapPoints();
        void CreateInitialMapMonocular();

        void CheckReplacedInLastFrame();
        bool TrackReferenceKeyFrame();
        void UpdateLastFrame();
        bool TrackWithMotionModel();
        bool PredictStateIMU();

        bool Relocalization();

        void UpdateLocalMap();
        void UpdateLocalPoints();
        void UpdateLocalKeyFrames();

        bool TrackLocalMap();
        void SearchLocalPoints();

        bool NeedNewKeyFrame();
        void CreateNewKeyFrame();

        // Perform preintegration from last frame
        void PreintegrateIMU();

        // Reset IMU biases and compute frame velocity
        void ResetFrameIMU();

        bool mbMapUpdated;

        // Imu preintegration from last frame
        IMU::Preintegrated *mpImuPreintegratedFromLastKF;

        // Queue of IMU measurements between frames
        std::list<IMU::Point> mlQueueImuData;

        // Vector of IMU measurements from previous to current frame (to be filled by PreintegrateIMU)
        std::vector<IMU::Point> mvImuFromLastFrame;
        std::mutex mMutexImuQueue;

        // Imu calibration parameters
        IMU::Calib *mpImuCalib;

        // Last Bias Estimation (at keyframe creation)
        IMU::Bias mLastBias;

        // In case of performing only localization, this flag is true when there are no matches to
        // points in the map. Still tracking will continue if there are enough matches with temporal points.
        // In that case we are doing visual odometry. The system will try to do relocalization to recover
        // "zero-drift" localization to the map.
        bool mbVO;

        // Other Thread Pointers
        LocalMapping *mpLocalMapper;
        LoopClosing *mpLoopClosing;

        // ORB
        ORBextractor *mpORBextractorLeft, *mpORBextractorRight;
        ORBextractor *mpIniORBextractor;

        // BoW
        ORBVocabulary *mpORBVocabulary;
        KeyFrameDatabase *mpKeyFrameDB;

        // Initalization (only for monocular)
        bool mbReadyToInitializate;
        bool mbSetInit;

        // Local Map
        KeyFrame *mpReferenceKF;
        std::vector<KeyFrame *> mvpLocalKeyFrames;
        std::vector<MapPoint *> mvpLocalMapPoints;

        // System
        System *mpSystem;

        // Drawers
        Viewer *mpViewer;
        FrameDrawer *mpFrameDrawer;
        MapDrawer *mpMapDrawer;
        bool bStepByStep;

        // Atlas
        Atlas *mpAtlas;

        // Calibration matrix
        cv::Mat mK;
        Eigen::Matrix3f mK_;
        cv::Mat mDistCoef;
        float mbf;
        float mImageScale;

        float mImuFreq;
        double mImuPer;
        bool mInsertKFsLost;

        // New KeyFrame rules (according to fps)
        int mMinFrames;
        int mMaxFrames;

        int mnFirstImuFrameId;
        int mnFramesToResetIMU;

        // Threshold close/far points
        // Points seen as close by the stereo/RGBD sensor are considered reliable
        // and inserted from just one frame. Far points requiere a match in two keyframes.
        float mThDepth;

        // For RGB-D inputs only. For some datasets (e.g. TUM) the depthmap values are scaled.
        float mDepthMapFactor;

        // Current matches in frame
        int mnMatchesInliers;

        // Last Frame, KeyFrame and Relocalisation Info
        KeyFrame *mpLastKeyFrame;
        unsigned int mnLastKeyFrameId;
        unsigned int mnLastRelocFrameId;
        double mTimeStampLost;
        double time_recently_lost;

        unsigned int mnFirstFrameId;
        unsigned int mnInitialFrameId;
        unsigned int mnLastInitFrameId;

        bool mbCreatedMap;

        // Motion Model
        bool mbVelocity{false};
        Sophus::SE3f mVelocity;

        // Color order (true RGB, false BGR, ignored if grayscale)
        bool mbRGB;

        list<MapPoint *> mlpTemporalPoints;

        // int nMapChangeIndex;

        int mnNumDataset;

        ofstream f_track_stats;

        ofstream f_track_times;
        double mTime_PreIntIMU;
        double mTime_PosePred;
        double mTime_LocalMapTrack;
        double mTime_NewKF_Dec;

        GeometricCamera *mpCamera, *mpCamera2;

        int initID, lastID;

        Sophus::SE3f mTlr;

        void newParameterLoader(Settings *settings);

#ifdef REGISTER_LOOP
        bool Stop();

        bool mbStopped;
        bool mbStopRequested;
        bool mbNotStop;
        std::mutex mMutexStop;
#endif

    public:
        cv::Mat mImRight;
    };

} // namespace ORB_SLAM

#endif // TRACKING_H
