#pragma once

namespace amu {

    namespace Binarize {

        /* WolfJolion, adapted to opencv 2.4 (from http://liris.cnrs.fr/christian.wolf/software/binarize/)
           Please cite the following paper if you use this code:
           @InProceedings{WolfICPR2002V,
           Author         = {C. Wolf and J.-M. Jolion and F. Chassaing},
           Title          = {Text {L}ocalization, {E}nhancement and {B}inarization in {M}ultimedia {D}ocuments},
           BookTitle      = {Proceedings of the {I}nternational {C}onference on {P}attern {R}ecognition},
           Volume         = {2},
           Pages          = {1037-1040},
           year           = 2002,
           }
           */
        enum NiblackVersion 
        {
            NIBLACK=0,
            SAUVOLA,
            WOLFJOLION,
        };

        double calcLocalStats (cv::Mat &im, cv::Mat &map_m, cv::Mat &map_s, int winx, int winy) {

            double m,s,max_s, sum, sum_sq, foo;
            int wxh	= winx/2;
            int wyh	= winy/2;
            int x_firstth= wxh;
            int y_lastth = im.rows-wyh-1;
            int y_firstth= wyh;
            double winarea = winx*winy;

            max_s = 0;
            for	(int j = y_firstth ; j<=y_lastth; j++) 
            {
                // Calculate the initial window at the beginning of the line
                sum = sum_sq = 0;
                for	(int wy=0 ; wy<winy; wy++)
                    for	(int wx=0 ; wx<winx; wx++) {
                        foo = im.at<uchar>(j-wyh+wy,wx);
                        sum    += foo;
                        sum_sq += foo*foo;
                    }
                m  = sum / winarea;
                s  = sqrt ((sum_sq - (sum*sum)/winarea)/winarea);
                if (s > max_s)
                    max_s = s;
                map_m.at<uchar>( j,x_firstth) = m;
                map_s.at<uchar>( j,x_firstth) = s;

                // Shift the window, add and remove	new/old values to the histogram
                for	(int i=1 ; i <= im.cols-winx; i++) {

                    // Remove the left old column and add the right new column
                    for (int wy=0; wy<winy; ++wy) {
                        foo = im.at<uchar>(j-wyh+wy,i-1);
                        sum    -= foo;
                        sum_sq -= foo*foo;
                        foo = im.at<uchar>(j-wyh+wy,i+winx-1);
                        sum    += foo;
                        sum_sq += foo*foo;
                    }
                    m  = sum / winarea;
                    s  = sqrt ((sum_sq - (sum*sum)/winarea)/winarea);
                    if (s > max_s)
                        max_s = s;
                    map_m.at<uchar>( j,i+wxh) = m;
                    map_s.at<uchar>( j,i+wxh) = s;
                }
            }

            return max_s;
        }

        void NiblackSauvolaWolfJolion (cv::Mat im, cv::Mat output, NiblackVersion version=WOLFJOLION, double k=0.5, double dR=128) {

            int winy = (int) (2.0 * im.rows-1)/3;
            int winx = (int) im.cols-1 < winy ? im.cols-1 : winy;
            if (winx > 100) winx = winy = 40;

            double m, s, max_s;
            double th=0;
            double min_I, max_I;
            int wxh	= winx/2;
            int wyh	= winy/2;
            int x_firstth= wxh;
            int x_lastth = im.cols-wxh-1;
            int y_lastth = im.rows-wyh-1;
            int y_firstth= wyh;

            // Create local statistics and store them in a double matrices
            cv::Mat map_m = cv::Mat::zeros (im.rows, im.cols, CV_32F);
            cv::Mat map_s = cv::Mat::zeros (im.rows, im.cols, CV_32F);
            max_s = calcLocalStats (im, map_m, map_s, winx, winy);

            cv::minMaxLoc(im, &min_I, &max_I);

            cv::Mat thsurf (im.rows, im.cols, CV_32F);

            // Create the threshold surface, including border processing
            // ----------------------------------------------------

            for	(int j = y_firstth ; j<=y_lastth; j++) {

                // NORMAL, NON-BORDER AREA IN THE MIDDLE OF THE WINDOW:
                for	(int i=0 ; i <= im.cols-winx; i++) {

                    m  = map_m.at<uchar>( j,i+wxh);
                    s  = map_s.at<uchar>( j,i+wxh);

                    // Calculate the threshold
                    switch (version) {

                        case NIBLACK:
                            th = m + k*s;
                            break;

                        case SAUVOLA:
                            th = m * (1 + k*(s/dR-1));
                            break;

                        case WOLFJOLION:
                            th = m + k * (s/max_s-1) * (m-min_I);
                            break;

                        default:
                            //cerr << "Unknown threshold type in ImageThresholder::surfaceNiblackImproved()\n";
                            exit (1);
                    }

                    thsurf.at<uchar>(j,i+wxh) = th;

                    if (i==0) {
                        // LEFT BORDER
                        for (int i=0; i<=x_firstth; ++i)
                            thsurf.at<uchar>(j,i) = th;

                        // LEFT-UPPER CORNER
                        if (j==y_firstth)
                            for (int u=0; u<y_firstth; ++u)
                                for (int i=0; i<=x_firstth; ++i)
                                    thsurf.at<uchar>(u,i) = th;

                        // LEFT-LOWER CORNER
                        if (j==y_lastth)
                            for (int u=y_lastth+1; u<im.rows; ++u)
                                for (int i=0; i<=x_firstth; ++i)
                                    thsurf.at<uchar>(u,i) = th;
                    }

                    // UPPER BORDER
                    if (j==y_firstth)
                        for (int u=0; u<y_firstth; ++u)
                            thsurf.at<uchar>(u,i+wxh) = th;

                    // LOWER BORDER
                    if (j==y_lastth)
                        for (int u=y_lastth+1; u<im.rows; ++u)
                            thsurf.at<uchar>(u,i+wxh) = th;
                }

                // RIGHT BORDER
                for (int i=x_lastth; i<im.cols; ++i)
                    thsurf.at<uchar>(j,i) = th;

                // RIGHT-UPPER CORNER
                if (j==y_firstth)
                    for (int u=0; u<y_firstth; ++u)
                        for (int i=x_lastth; i<im.cols; ++i)
                            thsurf.at<uchar>(u,i) = th;

                // RIGHT-LOWER CORNER
                if (j==y_lastth)
                    for (int u=y_lastth+1; u<im.rows; ++u)
                        for (int i=x_lastth; i<im.cols; ++i)
                            thsurf.at<uchar>(u,i) = th;
            }
            //cerr << "surface created" << endl;


            for	(int y=0; y<im.rows; ++y) 
                for	(int x=0; x<im.cols; ++x) 
                {
                    if (im.at<uchar>(y,x) >= thsurf.at<uchar>(y,x))
                    {
                        output.at<uchar>(y,x) = 255;
                    }
                    else
                    {
                        output.at<uchar>(y,x) = 0;
                    }
                }
        }


        // http://nghiaho.com/uploads/code/opencv_connected_component/blob.cpp
        void DrawBlobs(cv::Mat& output, const std::vector < std::vector<cv::Point2i> > &blobs) {

            // Randomy color the blobs
            for(size_t i=0; i < blobs.size(); i++) {
                unsigned char r = 255 * (rand()/(1.0 + RAND_MAX));
                unsigned char g = 255 * (rand()/(1.0 + RAND_MAX));
                unsigned char b = 255 * (rand()/(1.0 + RAND_MAX));

                for(size_t j=0; j < blobs[i].size(); j++) {
                    int x = blobs[i][j].x;
                    int y = blobs[i][j].y;

                    output.at<cv::Vec3b>(y,x)[0] = b;
                    output.at<cv::Vec3b>(y,x)[1] = g;
                    output.at<cv::Vec3b>(y,x)[2] = r;
                }
            }
        }

        void FindBlobs(const cv::Mat &binary, std::vector < std::vector<cv::Point2i> > &blobs)
        {
            blobs.clear();

            // Fill the label_image with the blobs
            // 0  - background
            // 1  - unlabelled foreground
            // 2+ - labelled foreground

            cv::Mat label_image;
            binary.convertTo(label_image, CV_32SC1);

            int label_count = 2; // starts at 2 because 0,1 are used already

            for(int y=0; y < label_image.rows; y++) {
                int *row = (int*)label_image.ptr(y);
                for(int x=0; x < label_image.cols; x++) {
                    if(row[x] != 1) {
                        continue;
                    }

                    cv::Rect rect;
                    cv::floodFill(label_image, cv::Point(x,y), label_count, &rect, 0, 0, 4);

                    std::vector <cv::Point2i> blob;

                    for(int i=rect.y; i < (rect.y+rect.height); i++) {
                        int *row2 = (int*)label_image.ptr(i);
                        for(int j=rect.x; j < (rect.x+rect.width); j++) {
                            if(row2[j] != label_count) {
                                continue;
                            }

                            blob.push_back(cv::Point2i(j,i));
                        }
                    }

                    blobs.push_back(blob);

                    label_count++;
                }
            }
        }


    }
}
