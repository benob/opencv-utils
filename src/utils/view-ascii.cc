#include "repere.h"
#include "font.h"
#include "commandline.h"

const char* mapping[256] = {
" ", "☺", "☻", "♥", "♦", "♣", "♠", "•", "◘", "○", "◙", "♂", "♀", "♪", "♫", "☼",
"►", "◄", "↕", "‼", "¶", "§", "▬", "↨", "↑", "↓", "→", "←", "∟", "↔", "▲", "▼",
" ", "!", "\"", "#", "$", "%", "&", "'", "(", ")", "*", "+", ",", "-", ".", "/",
"0", "1", "2", "3", "4", "5", "6", "7", "8", "9", ":", ";", "<", "=", ">", "?",
"@", "A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M", "N", "O",
"P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z", "[", "\\", "]", "^", "_",
"`", "a", "b", "c", "d", "e", "f", "g", "h", "i", "j", "k", "l", "m", "n", "o",
"p", "q", "r", "s", "t", "u", "v", "w", "x", "y", "z", "{", "|", "}", "~", "⌂",
"Ç", "ü", "é", "â", "ä", "à", "å", "ç", "ê", "ë", "è", "ï", "î", "ì", "Ä", "Å",
"É", "æ", "Æ", "ô", "ö", "ò", "û", "ù", "ÿ", "Ö", "Ü", "¢", "£", "¥", "₧", "ƒ",
"á", "í", "ó", "ú", "ñ", "Ñ", "ª", "º", "¿", "⌐", "¬", "½", "¼", "¡", "«", "»",
"░", "▒", "▓", "│", "┤", "╡", "╢", "╖", "╕", "╣", "║", "╗", "╝", "╜", "╛", "┐",
"└", "┴", "┬", "├", "─", "┼", "╞", "╟", "╚", "╔", "╩", "╦", "╠", "═", "╬", "╧",
"╨", "╤", "╥", "╙", "╙", "╘", "╒", "╓", "╫", "╪", "┘", "┌", "█", "▄", "▌", "▐",
"▀", "α", "ß", "Γ", "π", "Σ", "σ", "µ", "τ", "Φ", "Θ", "Ω", "δ", "∞", "φ", "ε",
"∩", "≡", "±", "≥", "≤", "⌠", "⌡", "÷", "≈", "°", "∙", "·", "√", "ⁿ", "²", "■"
};

inline int bgr2ansi(cv::Vec3b color) {
    if(fabs(color[2] - color[1]) + fabs(color[2] - color[0]) + fabs(color[1] - color[0]) < 12)
        return (color[0] + color[1] + color[2]) * 24 / (3 * 256) + 232;
    int r2 = color[2] * 6 / 256;
    int g2 = color[1] * 6 / 256;
    int b2 = color[0] * 6 / 256;
    return r2 * 36 + g2 * 6 + b2 + 16;
}

inline cv::Vec3b ansi2bgr(int color) {
    int r, g, b;
    if(color < 232) {
        color -= 16;
        r = (color / 36) % 6;
        g = (color / 6) % 6;
        b = color % 6;
        return cv::Vec3b(b, g, r);
    } else {
        int intensity = (color - 232) * 24; 
        return cv::Vec3b(intensity, intensity, intensity);
    }
}

inline int bgr2ansi(cv::Scalar color) {
    cv::Vec3b c;
    if(color[0] > 255) color[0] = 255;
    if(color[1] > 255) color[1] = 255;
    if(color[2] > 255) color[2] = 255;
    if(color[0] < 0) color[0] = 0;
    if(color[1] < 0) color[1] = 0;
    if(color[2] < 0) color[2] = 0;
    c[0] = (uchar) color[0];
    c[1] = (uchar) color[1];
    c[2] = (uchar) color[2];
    return bgr2ansi(c);
}

void drawFont2() {
    cv::Mat chars(cv::Size(16 * 8, 16 * 12), CV_8UC1);
    chars.setTo(0);
    for(int x = 0; x < 16; x++) {
        for(int y = 0; y < 16; y++) {
            int c = x + y * 16;
            std::string text = " ";
            text[0] = c;
            putText(chars, text, cv::Point(x * 8, (y + 1) * 12), cv::FONT_HERSHEY_DUPLEX, 0.5, 255, 1);
        }
    }
    cv::imshow("chars", chars);
    cv::waitKey(0);
}

int bestMatch(const cv::Mat binarized) {
    unsigned char current[12];

    for(int y = 0; y < 12; y++) {
        current[y] = 0;
        for(int x = 0; x < 8; x++) {
            if(binarized.at<uchar>(y, x) != 0) {
                current[y] |= 1 << (8 - x);
            }
        }
    }

    int min = 0;
    int argmin = -1;
    for(int c = 32; c < 128; c++) {
        int distance = 0;
        for(int y = 0; y < 12; y++) {
            int value = 0;
            for(int bit = 0; bit < 8; bit++) {
                value += (font[c * 12 + y] & (1 << bit)) ^ (current[y] & (1 << bit));
            }
            distance += value * value;
        }
        if(argmin == -1 || distance < min) {
            min = distance;
            argmin = c;
        }
    }
    return argmin;
}

int bestMatch2(const cv::Mat& image, int& output_fg, int& output_bg) {
    int mask[8] = {128, 64, 32, 16, 8, 4, 2, 1};
    int min = 0, argmin = -1;
    for(int c = 32; c < 128; c++) {
        int sumFg[3] = {0, 0, 0};
        int sumBg[3] = {0, 0, 0};
        int numFg = 0, numBg = 0;
        for(int y = 0; y < 12; y++) {
            for(int x = 0; x < 8; x++) {
                cv::Vec3b color = image.at<cv::Vec3b>(y, x);
                if(font[c * 12 + y] & mask[x]) {
                    sumFg[0] += color[0];
                    sumFg[1] += color[1];
                    sumFg[2] += color[2];
                    numFg ++;
                } else {
                    sumBg[0] += color[0];
                    sumBg[1] += color[1];
                    sumBg[2] += color[2];
                    numBg ++;
                }
            }
        }
        if(numFg == 0) numFg = 1;
        if(numBg == 0) numBg = 1;
        cv::Vec3b foreground(sumFg[0] / numFg, sumFg[1] / numFg, sumFg[2] / numFg);
        cv::Vec3b background(sumBg[0] / numBg, sumBg[1] / numBg, sumBg[2] / numBg);
        int fg = bgr2ansi(foreground), bg = bgr2ansi(background);
        foreground = ansi2bgr(fg);
        background = ansi2bgr(bg);
        int error = 0;
        for(int y = 0; y < 12; y++) {
            for(int x = 0; x < 8; x++) {
                cv::Vec3b color = image.at<cv::Vec3b>(y, x);
                if(font[c * 12 + y] & mask[x]) {
                    error += (color[0] - foreground[0]) * (color[0] - foreground[0]) + (color[1] - foreground[1]) * (color[1] - foreground[1]) + (color[2] - foreground[2]) * (color[2] - foreground[2]);
                } else {
                    error += (color[0] - background[0]) * (color[0] - background[0]) + (color[1] - background[1]) * (color[1] - background[1]) + (color[2] - background[2]) * (color[2] - background[2]);
                }
            }
        }
        if(argmin == -1 || error < min) {
            min = error;
            argmin = c;
            output_fg = fg;
            output_bg = bg;
        }
    }
    return argmin;
}

void drawFont() {
    cv::Mat chars(cv::Size(16 * 8, 16 * 12), CV_8UC1);
    chars.setTo(0);
    for(int x = 0; x < 16; x++) {
        for(int y = 0; y < 16; y++) {
            int c = x + y * 16;
            for(int line = 0; line < 12; line++) {
                int value = font[c * 12 + line];
                for(int col = 0; col < 8; col++) {
                    int pixel = (value >> col & (1 << (col - 1)));
                    chars.at<uchar>(y * 12 + line, x * 8 + col) = pixel * 255;
                }
            }
        }
    }
    cv::imshow("font", chars);
    cv::waitKey(0);
}

int main(int argc, char** argv) {
    amu::CommandLine options(argv, "[options] <images>\n");
    options.AddUsage("  --width <int>          max width in characters\n");
    options.AddUsage("  --height <int>         max height in characters\n");
    options.AddUsage("  --method 1|2|3         conversion method from pixels to ascii art");

    int max_width = options.Get<int>("--width", 80);
    int max_height = options.Get<int>("--height", 40);
    int method = options.Get<int>("--method", 1);

    if(method < 1 || method > 3 || options.Size() == 0) options.Usage();

    for(int i = 1; i < argc; i++) {
        std::cout << argv[i] << "\n";
        cv::Mat image = cv::imread(argv[i]);
        if(image.cols == 0 || image.rows == 0) continue;
        cv::Mat resized;
        int width, height;
        if(image.cols > image.rows) {
            width = max_width;
            height = image.rows * max_width / (image.cols * 2);
        } else {
            width = image.cols * max_height / (image.rows / 2);
            height = max_height;
        }
        cv::resize(image, resized, cv::Size(width * 8, height * 12), cv::INTER_AREA);
        cv::Mat gray, binarized;
        if(method == 1) {
            cv::cvtColor(resized, gray, CV_BGR2GRAY);
            cv::blur(gray, gray, cv::Size(3, 3), cv::Point(-1, -1));
        }
        for(int y = 0; y < height; y++) {
            for(int x = 0; x < width; x++) {
                cv::Rect rect(x * 8, y * 12, 8, 12);
                int fg, bg;
                int found;
                if(method == 1) {
                    cv::threshold(gray(rect), binarized, 0, 255, cv::THRESH_BINARY | cv::THRESH_OTSU);
                    found = bestMatch(binarized);
                    fg = bgr2ansi(cv::mean(resized(rect), binarized));
                    bg = bgr2ansi(cv::mean(resized(rect), ~binarized));
                } else if(method == 2) {
                    found = bestMatch2(resized(rect), fg, bg);
                } else if(method == 3) {
                    found = 256 - 35;
                    fg = bgr2ansi(cv::mean(resized(cv::Rect(x * 8, y * 12 + 6, 8, 6))));
                    bg = bgr2ansi(cv::mean(resized(cv::Rect(x * 8, y * 12, 8, 6))));
                }
                std::cout << "\033[48;5;" << (bg) << "m";
                std::cout << "\033[38;5;" << (fg) << "m";
                std::cout << mapping[found];
            }
            std::cout << "\033[0m\n";
        }
    }
}
