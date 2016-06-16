#include "spliter.h"


Spliter::Spliter(std::string save_dir)
{
    this->save_dir = save_dir;
}

void Spliter::split_letters(std::string filename)
{
    //read the image
    cv::Mat image =  cv::imread (filename, CV_LOAD_IMAGE_COLOR);
    cv::imshow ("init", image);

    // clean noise line
    this->clean_noise(image);

    //split the image
    int splits[8];
    captcha_utils.vertical_project (image, splits);
//    cv::Mat clone_mat = image.clone();
//    for(int i = 0; i < 8; i++){
//        std::cout<<i<<" "<<splits[i]<<std::endl;
//        cv::line (clone_mat, cv::Point(splits[i], 0), cv::Point(splits[i], 20), cv::Scalar(0));
//    }
//    cv::imshow ("split", clone_mat);

    if(splits[7] < image.size().width){//assume the final split is in the range of image.
        for(int i = 0; i < 8; i += 2){
            cv::Rect rect(cv::Point(splits[i], 0), cv::Point(splits[i+1], 20));
            cv::Mat mat = image(rect);
            this->save_image(mat);
        }
    }
}

void Spliter::save_image(cv::Mat &splited_mat)
{
    if(splited_mat.size().width > 30) return;

    //put the mat in center of 20*30 pixels image.
    int out_height = 20;
    int out_width = 30;
    cv::Mat out(out_height, out_width, CV_8UC1,cv::Scalar(255));
    int offset_x = (out_width - splited_mat.size().width)/2;
    cv::Mat imageROI;
    imageROI = out(cv::Rect(offset_x, 0, splited_mat.cols, splited_mat.rows));
    splited_mat.copyTo(imageROI);

    //generate random filename
    boost::uuids::random_generator rgen;
    boost::uuids::uuid ranUUID = rgen();
    std::string filename = this->save_dir + boost::lexical_cast<std::string>(ranUUID) + ".png";
    cv::imwrite(filename, out);
}


bool is_black(int i, int j, cv::Mat &image);
void clear_color(cv::Mat& image);
int get_horizontal_noise_line_width(cv::Mat &image, int now_height, int now_width);
void clear_horizontal_noise_line(cv::Mat &image);

void Spliter::clean_noise(cv::Mat &image)
{
    //clean noise line
    cv::flip (image, image, -1);
    clear_horizontal_noise_line(image);
    cv::flip (image, image, -1);
    clear_horizontal_noise_line(image);
    clear_color(image);
//    cv::imshow("clear", image);

    //threshold the image
    cv::cvtColor(image, image, CV_BGR2GRAY);//BGR to Gray
    cv::threshold (image, image, 150, 255, cv::THRESH_BINARY);
//    cv::imshow ("thresholded", image);

    //clean peper noise
    captcha_utils.clear_peper_noise(image, 2);
//    cv::imshow("clear peper:", image);
}

bool is_black(int i, int j, cv::Mat &image)
{
    int b =  image.at<cv::Vec3b>(j, i)[0];
    int g =  image.at<cv::Vec3b>(j, i)[1];
    int r =  image.at<cv::Vec3b>(j, i)[2];
    int average = (r + g + b)/3;
    if( r <244 &&  (abs(average-b) < 4)
        && (abs(average-g) < 4)
        && (abs(average-r) < 4)){
        return true;
    }
    return false;
}


void clear_color(cv::Mat& image)
{
    for(int i = 0; i < image.size().width; i++){
        for(int j = 0; j < image.size().height; j++){
           if(is_black(i, j, image)){
               image.at<cv::Vec3b>(j, i)[0] = 20;
               image.at<cv::Vec3b>(j, i)[1] = 20;
               image.at<cv::Vec3b>(j, i)[2] = 20;
           }
        }
    }
}


int get_horizontal_noise_line_width(cv::Mat &image, int now_height, int now_width)
{
    using namespace std;
    int end_width = now_width;
    while(end_width < image.size().width
          && image.at<cv::Vec3b>(now_height, end_width)[0] < 12
          && image.at<cv::Vec3b>(now_height, end_width)[1] < 12
          && image.at<cv::Vec3b>(now_height, end_width)[2] < 12){
//        cout<<
//          int(image.at<cv::Vec3b>(now_height, end_width)[0])<< " "<<
//          int(image.at<cv::Vec3b>(now_height, end_width)[1])<< " "<<
//          int(image.at<cv::Vec3b>(now_height, end_width)[2])<< " "<<endl;
        end_width++;
    }
    return end_width - now_width;
}



void clear_horizontal_noise_line(cv::Mat &image)
{
    using namespace std;
    int first_height;
    bool has_find = false;
    for(int i = 0; i < image.size().height; i++){
        //水平上连续三个点都是很黑的
        if(image.at<cv::Vec3b>(i, 0)[0] < 12
                && image.at<cv::Vec3b>(i, 0)[1] < 12
                && image.at<cv::Vec3b>(i, 0)[2] < 12
                && get_horizontal_noise_line_width(image, i, 0) >= 2 ){
            first_height = i;
            has_find = true;
        }
    }
    if(!has_find) return;
    //cout<<"first:"<<first_height<<endl;
    int now_width = 0;
    int now_height = first_height;
    while(now_width < image.size().width){
        int width = get_horizontal_noise_line_width(image, now_height, now_width);
        //cout<<now_width<<"  "<<now_height<< " width:"<<width<<endl;
        //清除直线
        for(int i = now_width; i < now_width + width; i++){
            int top_num = 0;
            int bottom_num = 0;
            //上面的点
            if(is_black(i, now_height-1, image)) top_num++;
            //左上面的点
            if(is_black(i-1, now_height-1, image)) top_num++;
            //右上面的点
            if(is_black(i+1, now_height-1, image)) top_num++;
            //下面的点
            if(is_black(i, now_height+1, image)) bottom_num++;
            //左下面的点
            if(is_black(i-1, now_height+1, image)) bottom_num++;
            //右下面的点
            if(is_black(i+1, now_height+1, image)) bottom_num++;

            if(now_height != 0 && now_height != image.size().height){
                if(top_num>0 && bottom_num>0) continue;
            }

            image.at<cv::Vec3b>(now_height, i)[0] = 255;
            image.at<cv::Vec3b>(now_height, i)[1] = 255;
            image.at<cv::Vec3b>(now_height, i)[2] = 255;
        }

        //寻找下一个点的位置
        int a = get_horizontal_noise_line_width(image, now_height - 1, now_width + width -1);
        int b = get_horizontal_noise_line_width(image, now_height + 1, now_width + width -1);
        int c = get_horizontal_noise_line_width(image, now_height - 1, now_width + width);
        int d = get_horizontal_noise_line_width(image, now_height + 1, now_width + width);
        if(now_height == 0) a=0,c=0;
        if(now_height == (image.size().height - 1) ) b=0,d=0;
        //cout<<"abcd: "<<a<<" "<<b<<" "<<c<<" "<<d<<endl;

        int max_a_b =  a>b?a:b;
        int max_c_d =  c>d?c:d;
        int max_a_b_c_d = max_a_b>max_c_d?max_a_b:max_c_d;
        //cout<<"max_abcd:"<<max_a_b_c_d<<endl;
        if(max_a_b_c_d < 2) break;
        if(max_a_b == max_a_b_c_d){//下一个点在正上方或者在正下方
            now_width += width-1;
            if(max_a_b == a){
                now_height -= 1;
            }else{
                now_height += 1;
            }
        }else{//下一个点在斜着的方法
            now_width += width;
            if(max_c_d == c){
                now_height -= 1;
            }else{
                now_height += 1;
            }
        }
    }
}


