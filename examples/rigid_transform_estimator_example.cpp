#include <cilantro/point_cloud.hpp>
#include <cilantro/rigid_transform_estimator.hpp>

#include <cilantro/ply_io.hpp>
#include <cilantro/visualizer.hpp>

#include <iostream>

bool re_estimate = false;
bool randomize = true;

void callback(Visualizer &viz, int key, void *cookie) {
    if (key == 'a') {
        re_estimate = true;
    }
    if (key == 'd') {
        randomize = true;
    }
}

int main(int argc, char **argv) {

    PointCloud dst;
    readPointCloudFromPLYFile(argv[1], dst);

    PointCloud src(dst);

    Visualizer viz("win", "disp");
    std::vector<int> keys;
    keys.push_back('a');
    keys.push_back('d');
    viz.registerKeyboardCallback(keys, callback, NULL);

    RigidTransformParameters tform;
    std::vector<size_t> inliers;

    std::vector<size_t> dst_ind(100);
    std::vector<size_t> src_ind(100);

    viz.addPointCloud("dst", dst, Visualizer::RenderingProperties().setDrawingColor(0,0,1));
    while (!viz.wasStopped()) {
        if (randomize) {
            randomize = false;

            Eigen::Matrix3f R_ref(Eigen::Matrix3f::Random());
            Eigen::JacobiSVD<Eigen::Matrix3f> svd(R_ref, Eigen::ComputeFullU | Eigen::ComputeFullV);
            Eigen::Matrix3f U(svd.matrixU());
            Eigen::Matrix3f Vt(svd.matrixV().transpose());
            Eigen::Matrix3f tmp(U * Vt);
            if (tmp.determinant() < 0) {
                Eigen::Matrix3f S(Eigen::Matrix3f::Identity());
                S(2, 2) = -1;
                R_ref = U * S * Vt;
            } else {
                R_ref = tmp;
            }
            Eigen::Vector3f t_ref(Eigen::Vector3f::Random());

            for (size_t i = 0; i < dst_ind.size(); i++) {
                float p = 0 + static_cast <float> (rand()) /( static_cast <float> (RAND_MAX/(1-0)));
                src_ind[i] = std::rand() % src.size();
                if (p > 0.4) {
                    dst_ind[i] = src_ind[i];
                } else {
                    dst_ind[i] = std::rand() % dst.size();
                }
            }

            src.pointsMatrixMap() = (R_ref*src.pointsMatrixMap()).colwise() + t_ref;
            viz.addPointCloud("src", src, Visualizer::RenderingProperties().setDrawingColor(1,0,0));
        }

        if (re_estimate) {
            re_estimate = false;

            RigidTransformEstimator te(dst,src,dst_ind,src_ind);
            te.setMaxInlierResidual(0.01).setTargetInlierCount((size_t)(0.50*dst_ind.size())).setMaxNumberOfIterations(250).setReEstimationStep(true);
            te.getModel(tform, inliers);
            std::cout << "RANSAC iterations: " << te.getPerformedIterationsCount() << ", inlier count: " << te.getNumberOfInliers() << std::endl;

            src.pointsMatrixMap() = (tform.rotation*src.pointsMatrixMap()).colwise() + tform.translation;

            viz.addPointCloud("src", src, Visualizer::RenderingProperties().setDrawingColor(1,0,0));
        }

        viz.spinOnce();
    }

    return 0;
}
