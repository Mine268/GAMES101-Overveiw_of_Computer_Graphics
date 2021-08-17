#include <eigen3/Eigen/Eigen>
#include <iostream>
#include <opencv2/opencv.hpp>

#include "Triangle.hpp"
#include "rasterizer.hpp"

constexpr double MY_PI = 3.1415926;

Eigen::Matrix4f get_view_matrix(Eigen::Vector3f eye_pos) {
	Eigen::Matrix4f view = Eigen::Matrix4f::Identity();

	Eigen::Matrix4f translate;
	translate << 1, 0, 0, -eye_pos[0], 0, 1, 0, -eye_pos[1], 0, 0, 1,
		-eye_pos[2], 0, 0, 0, 1;

	view = translate * view;

	return view;
}

Eigen::Matrix4f get_model_matrix(float rotation_angle) {
	Eigen::Matrix4f model = Eigen::Matrix4f::Identity();

	// TODO: Implement this function
	// Create the model matrix for rotating the triangle around the Z axis.
	// Then return it.
	model << cosf(rotation_angle), -sinf(rotation_angle), 0, 0,
		sinf(rotation_angle), cos(rotation_angle), 0, 0, 0, 0, 1, 0, 0, 0, 0, 1;

	return model;
}

Eigen::Matrix4f get_model_matrix(Eigen::Vector4f axis, float rotation_angle) {
	// TODO: Implement this function
	// Create the model matrix for rotating the trangle around axis vector
	// for rotation_angle angle.
	// Then return it.

	Eigen::MatrixXf K(3, 3), model(3, 3);
	axis.normalize();
	K << 0, -axis.z(), axis.y(), axis.z(), 0, axis.x(), -axis.y(), axis.x(), 0;
	model = Eigen::Matrix3f::Identity() + (1 - cosf(rotation_angle)) * K * K +
			K * sinf(rotation_angle);
	model.conservativeResize(4, 4);
	model.col(3) << 0, 0, 0, 1;
	model.row(3) << 0, 0, 0, 1;

	return model;
}

Eigen::Matrix4f get_projection_matrix(float eye_fov, float aspect_ratio,
									  float zNear, float zFar) {
	// Students will implement this function

	Eigen::Matrix4f projection = Eigen::Matrix4f::Identity();
	Eigen::Matrix4f orthographics1 = Eigen::Matrix4f::Identity();
	Eigen::Matrix4f orthographics2 = Eigen::Matrix4f::Identity();

	float l, r, t, b, n, f;
	n = -zNear;
	f = -zFar;
	t = tanf(eye_fov / 2.) * zNear;
	b = -t;
	r = t / aspect_ratio;
	l = -r;

	// TODO: Implement this function
	// Create the projection matrix for the given parameters.
	// Then return it.

	projection << zNear, 0, 0, 0, 0, zNear, 0, 0, 0, 0, zNear + zFar,
		-zNear * zFar, 0, 0, -1, 0;
	orthographics1 << 2. / (r - l), 0, 0, 0, 0, 2. / (t - b), 0, 0, 0, 0,
		2. / (n - f), 0, 0, 0, 0, 1;
	orthographics2 << 1., 0, 0, (l + r) / 2., 0, 1., 0, (t + b) / 2., 0, 0, 1., (n + f) / 2., 0, 0,
		0, 1.;

	return orthographics1 * orthographics2 * projection;
}

int main(int argc, const char** argv) {
	float angle = 0;
	bool command_line = false;
	std::string filename = "output.png";

	if (argc >= 3) {
		command_line = true;
		angle = std::stof(argv[2]);	 // -r by default
		if (argc == 4) {
			filename = std::string(argv[3]);
		} else
			return 0;
	}

	rst::rasterizer r(700, 700);

	Eigen::Vector3f eye_pos = {0, 0, 5};

	std::vector<Eigen::Vector3f> pos{{2, 0, -2}, {0, 2, -2}, {-2, 0, -2}};

	std::vector<Eigen::Vector3i> ind{{0, 1, 2}};

	auto pos_id = r.load_positions(pos);
	auto ind_id = r.load_indices(ind);

	int key = 0;
	int frame_count = 0;

	if (command_line) {
		r.clear(rst::Buffers::Color | rst::Buffers::Depth);

		r.set_model(get_model_matrix(angle));
		r.set_view(get_view_matrix(eye_pos));
		r.set_projection(get_projection_matrix(MY_PI / 4., 1, 0.1, 50));

		r.draw(pos_id, ind_id, rst::Primitive::Triangle);
		cv::Mat image(700, 700, CV_32FC3, r.frame_buffer().data());
		image.convertTo(image, CV_8UC3, 1.0f);

		cv::imwrite(filename, image);

		return 0;
	}

	while (key != 27) {
		r.clear(rst::Buffers::Color | rst::Buffers::Depth);

		r.set_model(get_model_matrix(angle));
		r.set_view(get_view_matrix(eye_pos));
		r.set_projection(get_projection_matrix(MY_PI / 2, 1, 0.1, 50));

		r.draw(pos_id, ind_id, rst::Primitive::Triangle);

		cv::Mat image(700, 700, CV_32FC3, r.frame_buffer().data());
		image.convertTo(image, CV_8UC3, 1.0f);
		cv::imshow("image", image);
		key = cv::waitKey(10);

		std::cout << "angle " << angle << '\n';
        ++frame_count;

		if (key == 'a') {
			angle += .1;
		} else if (key == 'd') {
			angle -= .1;
		}
	}

	return 0;
}
