#ifndef ImageManipulator_HPP
#define ImageManipulator_HPP

class ImageManipulator {

public:
    ImageManipulator(std::string filename);
    void reduce(int width, int height);

private:
    std::string filename_;
};

#endif // ImageManipulator_HPP