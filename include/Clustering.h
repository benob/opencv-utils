#pragma once
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <cv.h>
#include <highgui.h>
#include <math.h>

namespace amu {

struct Cluster {
    int size;
    int index;
    cv::Mat sum;
    double rank;
    Cluster* first;
    Cluster* second;
    std::vector<int> members;

    Cluster(Cluster* _first, Cluster* _second, double _rank) {
        size = _first->size + _second->size;
        index = -1;
        sum = cv::Mat::zeros(_first->sum.rows, _first->sum.cols, CV_32F);
        sum += _first->sum;
        sum += _second->sum;
        rank = _rank;
        first = _first;
        second = _second;
    }

    Cluster(int _index, const cv::Mat& example, int _size = 1) {
        size = _size;
        index = _index;
        example.copyTo(sum);
        rank = 0;
        first = NULL;
        second = NULL;
    }

    ~Cluster() {
        if(first != NULL) delete first;
        if(second != NULL) delete second;
    }

    void CollectMembers(std::vector<int>& _members) const {
        if(index != -1) {
            _members.push_back(index);
        } else {
            if(first) first->CollectMembers(_members);
            if(second) second->CollectMembers(_members);
        }
    }

    void CollectClusters(std::vector<Cluster*>& output, double threshold = 0) {
        if(rank < threshold) {
            members.clear();
            CollectMembers(members);
            output.push_back(this);
        } else {
            if(first) first->CollectClusters(output, threshold);
            if(second) second->CollectClusters(output, threshold);
        }
    }

    static double Ward(const Cluster* a, const Cluster* b) {
        cv::Mat a_center = a->sum.mul(1.0 / a->size);
        cv::Mat b_center = b->sum.mul(1.0 / b->size);
        cv::Mat difference = a_center - b_center;
        return (double) (difference.dot(difference) * a->size * b->size) / (a->size + b->size);
    }

    static double Distance(const cv::Mat& a, const cv::Mat& b) {
        return cv::norm(a - b);
    }

    double Diameter(const cv::Mat& examples) {
        if(members.size() == 0) CollectMembers(members);
        double diameter = 0;
        for(size_t i = 0; i < members.size(); i++) {
            for(size_t j = 0; j < members.size(); j++) {
                if(i == j) continue;
                cv::Mat example1, example2;
                examples.row(members[i]).convertTo(example1, CV_32F);
                examples.row(members[j]).convertTo(example2, CV_32F);
                double distance = Distance(example1.mul(1.0 / 256), example2.mul(1.0 / 256));
                if(distance > diameter) diameter = distance;
            }
        }
        return diameter;
    }

    double Radius(const cv::Mat& examples) {
        if(members.size() == 0) CollectMembers(members);
        double radius = 0;
        cv::Mat center = sum.mul(1.0 / (size * 256));
        for(size_t i = 0; i < members.size(); i++) {
            cv::Mat normalized;
            examples.row(members[i]).convertTo(normalized, CV_32F);
            normalized = normalized.mul(1.0 / 256);
            double distance = Distance(center, normalized);
            if(distance > radius) radius = distance;
        }
        return radius;
    }

    static Cluster* Compute(const cv::Mat& examples, const std::vector<int>& sizes) {
        Cluster* clusters[examples.rows];
        double distanceCache[examples.rows][examples.rows];
        for(int i = 0; i < examples.rows; i++) {
            clusters[i] = new Cluster(i, examples.row(i), sizes[i]);
            for(int j = 0; j < examples.rows; j++) distanceCache[i][j] = -1;
        }
        int k;
        for(k = examples.rows - 1; k > 0; k--) {
            int argmin1 = -1, argmin2 = -1;
            double min = 0;
            for(int i = 0; i <= k; i++) {
                for(int j = 0; j <= k; j++) {
                    if(i >= j) continue;
                    if(distanceCache[i][j] == -1) distanceCache[i][j] = Cluster::Ward(clusters[i], clusters[j]);
                    double distance = distanceCache[i][j];
                    if(argmin1 == -1 || min > distance) {
                        min = distance;
                        argmin1 = i;
                        argmin2 = j;
                    }
                }
            }
            std::cout << min << " " << argmin1 << " " << argmin2 << "\n";
            clusters[argmin1] = new Cluster(clusters[argmin1], clusters[argmin2], min);
            clusters[argmin2] = clusters[k];
            for(int i = 0; i < k; i++) {
                distanceCache[argmin2][i] = distanceCache[k][i];
                distanceCache[i][argmin2] = distanceCache[i][k];
                distanceCache[argmin1][i] = -1;
                distanceCache[i][argmin1] = -1;
            }
        }
        return clusters[0];
    }
};

}
