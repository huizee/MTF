#ifndef MTF_MISC_UTILS_H
#define MTF_MISC_UTILS_H

#include "mtf/Macros/common.h"

_MTF_BEGIN_NAMESPACE

namespace utils{

	/**
	compute the rectangle that best fits an arbitrry quadrilateral
	in terms of maximizing the Jaccard index of overlap 
	between the corresponding regions; 
	an optional resize factor is provided to avaoid further loss in precision
	in case a resizing is needed and the input corners are floating point numbers;	
	*/ 
	template<typename ValT>
	cv::Rect_<ValT> getBestFitRectangle(const cv::Mat &corners,
		int img_width = 0, int img_height = 0, int border_size = 0);
	//! adjust the rectangle bounds so it lies entirely within the image with the given size
	template<typename ValT>
	cv::Rect_<ValT> getBoundedRectangle(const cv::Rect_<ValT> &_in_rect, int img_width, int img_height,
		int border_size = 0);

	//! convert region corners between various formats
	struct Corners{
		template<typename PtScalarT>
		Corners(const cv::Point_<PtScalarT>(&pt_corners)[4],
			PtScalarT offset_x = 0, PtScalarT offset_y = 0){
			corners.create(2, 4, CV_64FC1);
			for(int corner_id = 0; corner_id < 4; corner_id++) {
				corners.at<double>(0, corner_id) = pt_corners[corner_id].x + offset_x;
				corners.at<double>(1, corner_id) = pt_corners[corner_id].y + offset_y;
			}
		}
		template<typename RectScalarT>
		Corners(const cv::Rect_<RectScalarT> rect,
			RectScalarT offset_x = 0, RectScalarT offset_y = 0){
			corners.create(2, 4, CV_64FC1);
			RectScalarT min_x = rect.x + offset_x, min_y = rect.y + offset_y;
			RectScalarT max_x = min_x + rect.width, max_y = min_y + rect.height;
			corners.at<double>(0, 0) = corners.at<double>(0, 3) = min_x;
			corners.at<double>(0, 1) = corners.at<double>(0, 2) = max_x;
			corners.at<double>(1, 0) = corners.at<double>(1, 1) = min_y;
			corners.at<double>(1, 2) = corners.at<double>(1, 3) = max_y;
		}
		Corners(const cv::Mat &mat_corners,
			double offset_x = 0, double offset_y = 0){
			corners = mat_corners.clone();
			corners.row(0) += offset_x;
			corners.row(1) += offset_y;
		}
		Corners(const CornersT &eig_corners,
			double offset_x = 0, double offset_y = 0){
			corners.create(2, 4, CV_64FC1);
			for(int corner_id = 0; corner_id < 4; ++corner_id){
				corners.at<double>(0, corner_id) = eig_corners(0, corner_id) + offset_x;
				corners.at<double>(1, corner_id) = eig_corners(1, corner_id) + offset_y;
			}
		};
		template<typename RectScalarT>
		cv::Rect_<RectScalarT>  rect(){
			return getBestFitRectangle<RectScalarT>(corners);
		}
		template<typename PtScalarT>
		void points(cv::Point_<PtScalarT>(&pt_corners)[4]){
			for(int corner_id = 0; corner_id < 4; ++corner_id) {
				pt_corners[corner_id].x = corners.at<double>(0, corner_id);
				pt_corners[corner_id].y = corners.at<double>(1, corner_id);
			}
		}
		cv::Mat mat(){ return corners; }
		void mat(cv::Mat &mat_corners){ mat_corners = corners.clone(); }
		CornersT eig(){
			CornersT eig_corners;
			eig(eig_corners);
			return eig_corners;
		}
		void eig(CornersT &eig_corners){
			for(int corner_id = 0; corner_id < 4; ++corner_id){
				eig_corners(0, corner_id) = corners.at<double>(0, corner_id);
				eig_corners(1, corner_id) = corners.at<double>(1, corner_id);
			}
		}
	private:
		cv::Mat corners;
	};
	template<typename MatT>
	inline void printMatrix(const MatT &eig_mat, const char* mat_name = nullptr,
		const char* fmt = "%15.9f", const char *coeff_sep = "\t",
		const char *row_sep = "\n"){
		if(mat_name)
			printf("%s:\n", mat_name);
		for(int i = 0; i < eig_mat.rows(); i++){
			for(int j = 0; j < eig_mat.cols(); j++){
				printf(fmt, eig_mat(i, j));
				printf("%s", coeff_sep);
			}
			printf("%s", row_sep);
		}
		printf("\n");
	}
	template<typename ScalarT>
	inline void printScalar(ScalarT scalar_val, const char* scalar_name,
		const char* fmt = "%15.9f", const char *name_sep = "\t",
		const char *val_sep = "\n"){
		fprintf(stdout, "%s:%s", scalar_name, name_sep);
		fprintf(stdout, fmt, scalar_val);
		fprintf(stdout, "%s", val_sep);
	}
	// write matrix values to a custom formatted ASCII text file
	template<typename MatT>
	inline void printMatrixToFile(const MatT &eig_mat, const char* mat_name,
		const char* fname, const char* fmt = "%15.9f", const char* mode = "a",
		const char *coeff_sep = "\t", const char *row_sep = "\n",
		char** const row_labels = nullptr, const char **mat_header = nullptr,
		const char* header_fmt = "%15s", const char *name_sep = "\n"){
		//typedef typename ImageT::RealScalar ScalarT;
		//printf("Opening file: %s to write %s\n", fname, mat_name);
		FILE *fid = fopen(fname, mode);
		if(!fid){
			printf("File %s could not be opened successfully\n", fname);
			return;
		}
		if(mat_name)
			fprintf(fid, "%s:%s", mat_name, name_sep);
		if(mat_header){
			for(int j = 0; j < eig_mat.cols(); j++){
				fprintf(fid, header_fmt, mat_header[j]);
				fprintf(fid, "%s", coeff_sep);
			}
			fprintf(fid, "%s", row_sep);
		}
		for(int i = 0; i < eig_mat.rows(); i++){
			for(int j = 0; j < eig_mat.cols(); j++){
				fprintf(fid, fmt, eig_mat(i, j));
				fprintf(fid, "%s", coeff_sep);

			}
			if(row_labels){
				fprintf(fid, "\t%s", row_labels[i]);
			}
			fprintf(fid, "%s", row_sep);
		}
		fclose(fid);
	}
	// write scalar value to a custom formatted ASCII text file
	template<typename ScalarT>
	inline void printScalarToFile(ScalarT scalar_val, const char* scalar_name,
		const char* fname, const char* fmt = "%15.9f", const char* mode = "a",
		const char *name_sep = "\t", const char *val_sep = "\n"){
		//typedef typename ImageT::RealScalar ScalarT;
		FILE *fid = fopen(fname, mode);
		if(!fid){
			printf("File %s could not be opened successfully\n", fname);
			return;
		}
		if(scalar_name)
			fprintf(fid, "%s:%s", scalar_name, name_sep);
		fprintf(fid, fmt, scalar_val);
		fprintf(fid, "%s", val_sep);
		fclose(fid);
	}
	// save matrix data to binary file
	template<typename ScalarT, typename MatT>
	inline void saveMatrixToFile(const MatT &eig_mat, const char* fname,
		const char* mode = "ab"){
		FILE *fid = fopen(fname, "ab");
		if(!fid){
			printf("File %s could not be opened successfully\n", fname);
			return;
		}
		fwrite(eig_mat.data(), sizeof(ScalarT), eig_mat.size(), fid);
		fclose(fid);
	}
	// save scalar data to binary file
	template<typename ScalarT>
	inline void saveScalarToFile(ScalarT &scalar_val, const char* fname,
		const char* mode = "ab"){
		FILE *fid = fopen(fname, mode);
		if(!fid){
			printf("File %s could not be opened successfully\n", fname);
			return;
		}
		fwrite(&scalar_val, sizeof(ScalarT), 1, fid);
		fclose(fid);
	}

	// printing functions for OpenCV Mat matrices
	template<typename ScalarT>
	inline void printMatrix(const cv::Mat &cv_mat, const char* mat_name,
		const char* fmt = "%15.9f", const char *coeff_sep = "\t",
		const char *row_sep = "\n"){
		printf("%s:\n", mat_name);
		for(int i = 0; i < cv_mat.rows; i++){
			for(int j = 0; j < cv_mat.cols; j++){
				printf(fmt, cv_mat.at<ScalarT>(i, j));
				printf("%s", coeff_sep);
			}
			printf("%s", row_sep);
		}
		printf("\n");
	}

	template<typename ScalarT>
	inline void printMatrixToFile(const cv::Mat &cv_mat, const char* mat_name,
		const char* fname, const char* fmt = "%15.9f", const char* mode = "a",
		const char *coeff_sep = "\t", const char *row_sep = "\n",
		const char **row_labels = NULL, const char **mat_header = NULL,
		const char* header_fmt = "%15s"){
		//typedef typename ImageT::RealScalar ScalarT;
		//printf("Opening file: %s to write %s\n", fname, mat_name);
		FILE *fid = fopen(fname, mode);
		if(!fid){
			printf("File %s could not be opened successfully\n", fname);
			return;
		}
		fprintf(fid, "%s:\n", mat_name);
		if(mat_header){
			for(int j = 0; j < cv_mat.cols; j++){
				fprintf(fid, header_fmt, mat_header[j]);
				fprintf(fid, "%s", coeff_sep);
			}
			fprintf(fid, "%s", row_sep);
		}
		for(int i = 0; i < cv_mat.rows; i++){
			for(int j = 0; j < cv_mat.cols; j++){
				fprintf(fid, fmt, cv_mat.at<ScalarT>(i, j));
				fprintf(fid, "%s", coeff_sep);

			}
			if(row_labels){
				fprintf(fid, "\t%s", row_labels[i]);
			}
			fprintf(fid, "%s", row_sep);
		}
		fclose(fid);
	}

	template<typename MatT>
	inline bool hasInf(const MatT &eig_mat){
		for(int row_id = 0; row_id < eig_mat.rows(); row_id++){
			for(int col_id = 0; col_id < eig_mat.cols(); col_id++){
				if(std::isinf(eig_mat(row_id, col_id))){ return true; }
			}
		}
		return false;
	}
	template<typename MatT>
	inline bool hasNaN(const MatT &eig_mat){
		for(int row_id = 0; row_id < eig_mat.rows(); row_id++){
			for(int col_id = 0; col_id < eig_mat.cols(); col_id++){
				if(std::isnan(eig_mat(row_id, col_id))){ return true; }
			}
		}
		return false;
	}
	template<typename ScalarT>
	inline bool hasInf(const cv::Mat &cv_mat){
		for(int row_id = 0; row_id < cv_mat.rows; ++row_id){
			for(int col_id = 0; col_id < cv_mat.cols; ++col_id){
				if(std::isinf(cv_mat.at<ScalarT>(row_id, col_id))){ return true; }
			}
		}
		return false;
	}
	template<typename ScalarT>
	inline bool hasNaN(const cv::Mat &cv_mat){
		for(int row_id = 0; row_id < cv_mat.rows; ++row_id){
			for(int col_id = 0; col_id < cv_mat.cols; ++col_id){
				if(std::isnan(cv_mat.at<ScalarT>(row_id, col_id))){ return true; }
			}
		}
		return false;
	}
	template<typename ScalarT>
	inline bool isFinite(const cv::Mat &cv_mat){
		for(int row_id = 0; row_id < cv_mat.rows; ++row_id){
			for(int col_id = 0; col_id < cv_mat.cols; ++col_id){
				if(std::isnan(cv_mat.at<ScalarT>(row_id, col_id)) ||
					std::isinf(cv_mat.at<ScalarT>(row_id, col_id))){
					return false;
				}
			}
		}
		return true;
	}
	void drawCorners(cv::Mat &img, const cv::Point2d(&cv_corners)[4],
		const cv::Scalar corners_col, const std::string label);
	// mask a vector, i.e. retain only those entries where the given mask is true
	void maskVector(VectorXd &masked_vec, const VectorXd &in_vec,
		const VectorXb &mask, int masked_size, int in_size);
	// returning version
	VectorXd maskVector(const VectorXd &in_vec,
		const VectorXb &mask, int masked_size, int in_size);
	// mask 2D matrix by row, i.e. retain only those columns whee the mask is true
	template<typename MatT>
	inline void maskMatrixByRow(MatT &masked_mat, const MatT &in_mat,
		const VectorXb &mask, int n_cols){
		assert(in_mat.cols() == n_cols);
		assert(in_mat.cols() == mask.size());
		assert(masked_mat.rows() == in_mat.rows());

		int masked_size = mask.array().count();
		masked_mat.resize(NoChange, masked_size);
		int mask_id = 0;
		for(int i = 0; i < n_cols; i++){
			if(mask(i)){ masked_mat.col(mask_id++) = in_mat.col(i); }
		}
	}
	// returning version
	template<typename MatT>
	inline MatT maskMatrixByRow(const MatT &in_mat,
		const VectorXb &mask, int n_cols){
		int masked_size = mask.array().count();
		MatT masked_mat(in_mat.rows(), masked_size);
		maskMatrixByRow(masked_mat, in_mat, mask, n_cols);
		return masked_mat;
	}
	// mask 2D matrix by column, i.e. retain only those rows where the mask is true
	template<typename MatT>
	inline void maskMatrixByCol(MatT &masked_mat, const MatT &in_mat,
		const VectorXb &mask, int n_rows){
		assert(in_mat.rows() == n_rows);
		assert(in_mat.rows() == mask.size());
		assert(masked_mat.rows() == in_mat.rows());

		int masked_size = mask.array().count();
		masked_mat.resize(NoChange, masked_size);
		int mask_id = 0;
		for(int i = 0; i < n_rows; i++){
			if(mask(i)){ masked_mat.row(mask_id++) = in_mat.row(i); }
		}
	}
	// returning version
	template<typename MatT>
	inline MatT maskMatrixByCol(const MatT &in_mat,
		const VectorXb &mask, int n_rows){
		int masked_size = mask.array().count();
		MatT masked_mat(masked_size, in_mat.cols());
		maskMatrixByRow(masked_mat, in_mat, mask, n_rows);
		return masked_mat;
	}

	template<typename ScalarT, typename EigT>
	inline void copyCVToEigen(EigT &eig_mat, const cv::Mat &cv_mat){
		assert(eig_mat.rows() == cv_mat.rows && eig_mat.cols() == cv_mat.cols);
		for(int i = 0; i < cv_mat.rows; i++){
			for(int j = 0; j < cv_mat.cols; j++){
				eig_mat(i, j) = cv_mat.at<ScalarT>(i, j);
			}
		}
	}
	// specialization with loop unrolling for copying a warp matrix from OpenCV to Eigen
	template<>
	void copyCVToEigen<double, Matrix3d>(Matrix3d &eig_mat, const cv::Mat &cv_mat);
	//returning version
	template<typename ScalarT>
	inline MatrixXd copyCVToEigen(const cv::Mat &cv_mat){
		MatrixXd eig_mat(cv_mat.rows, cv_mat.cols);
		copyCVToEigen<MatrixXd, ScalarT>(eig_mat, cv_mat);
		return eig_mat;
	}

	template<typename ScalarT, typename EigT>
	inline void copyEigenToCV(cv::Mat &cv_mat, const  EigT &eig_mat){
		assert(cv_mat.rows == eig_mat.rows() && cv_mat.cols == eig_mat.cols());
		for(int i = 0; i < cv_mat.rows; i++){
			for(int j = 0; j < cv_mat.cols; j++){
				cv_mat.at<ScalarT>(i, j) = eig_mat(i, j);
			}
		}
	}
	// specialization for copying corners
	template<>
	void copyEigenToCV<double, CornersT>(cv::Mat &cv_mat,
		const CornersT &eig_mat);
	//returning version
	template<typename EigT, typename ScalarT, int CVMatT>
	inline cv::Mat copyEigenToCV(const EigT &eig_mat){
		cv::Mat cv_mat(eig_mat.rows(), eig_mat.cols(), CVMatT);
		copyEigenToCV<EigT, ScalarT>(cv_mat, eig_mat);
		return cv_mat;
	}
	int writeTimesToFile(vector<double> &proc_times,
		vector<char*> &proc_labels, char *time_fname, int iter_id);
	//! draw the boundary of the image region represented by the polygon formed by the specified vertices
	void drawRegion(cv::Mat &img, const cv::Mat &vertices, cv::Scalar col = cv::Scalar(0, 255, 0),
		int line_thickness = 2, const char *label = nullptr, double font_size = 0.50,
		bool show_corner_ids = false, bool show_label = false);
	void drawGrid(cv::Mat &img, const PtsT &grid_pts, int res_x, int res_y,
		cv::Scalar col = cv::Scalar(0, 255, 0), int thickness = 1);
	template<typename ImgValT, typename PatchValT>
	void drawPatch(cv::Mat &img, const cv::Mat &patch, int n_channels = 1, int start_x = 0, int start_y = 0);
	void writeCorners(FILE *out_fid, const cv::Mat &corners, int frame_id, bool write_header = false);

	//! functions to handle tracking error computation
	enum class TrackErrT{ MCD, CL, Jaccard };
	const char* toString(TrackErrT _er_type);
	template<TrackErrT tracking_err_type>
	double getTrackingError(const cv::Mat &gt_corners, const cv::Mat &tracker_corners){
		throw invalid_argument(cv::format("Invalid tracking error type provided: %d", tracking_err_type));
	}
	template<> double getTrackingError<TrackErrT::MCD>(const cv::Mat &gt_corners,
		const cv::Mat &tracker_corners);
	template<> double getTrackingError<TrackErrT::CL>(const cv::Mat &gt_corners,
		const cv::Mat &tracker_corners);
	template<> double getTrackingError<TrackErrT::Jaccard>(const cv::Mat &gt_corners,
		const cv::Mat &tracker_corners);
	double getJaccardError(const cv::Mat &gt_corners, const cv::Mat &tracker_corners,
		int img_width, int img_height);
	double getTrackingError(TrackErrT tracking_err_type,
		const cv::Mat &gt_corners, const cv::Mat &tracker_corners,
		FILE *out_fid = nullptr, int frame_id = 0,
		int img_width = 0, int img_height = 0);

	cv::Mat readTrackerLocation(const std::string &file_path);
	cv::Mat getFrameCorners(const cv::Mat &img, int borner_size = 1);
	mtf::PtsT getFramePts(const cv::Mat &img, int borner_size = 1);
	cv::Point2d getCentroid(const cv::Mat &corners);
	template<typename ScalarT>
	inline void getCentroid(cv::Point_<ScalarT> &centroid,
		const cv::Mat &corners){
		centroid.x = (corners.at<double>(0, 0) + corners.at<double>(0, 1)
			+ corners.at<double>(0, 2) + corners.at<double>(0, 3)) / 4.0;
		centroid.y = (corners.at<double>(1, 0) + corners.at<double>(1, 1)
			+ corners.at<double>(1, 2) + corners.at<double>(1, 3)) / 4.0;

	}
	inline void getCentroid(Vector2d &centroid,
		const cv::Mat &corners){
		centroid(0) = (corners.at<double>(0, 0) + corners.at<double>(0, 1)
			+ corners.at<double>(0, 2) + corners.at<double>(0, 3)) / 4.0;
		centroid(1) = (corners.at<double>(1, 0) + corners.at<double>(1, 1)
			+ corners.at<double>(1, 2) + corners.at<double>(1, 3)) / 4.0;

	}
	cv::Mat reshapePatch(const VectorXd &curr_patch,
		int img_height, int img_width, int n_channels = 1);
}
_MTF_END_NAMESPACE
#endif
