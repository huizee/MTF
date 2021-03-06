#include "mtf/SM/LineTracker.h"
#include "mtf/Utilities/miscUtils.h"

_MTF_BEGIN_NAMESPACE

gridLine::gridLine() {
	type = 0;
	inited = 0;
	params = nullptr;
	pts = nullptr;
}

void gridLine::init(int no_of_pts, int type) {
	if(no_of_pts < 2) {
		printf("Error in gridLine::init: Line cannot have less than 2 points\n");
		return;
	}
	this->no_of_pts = no_of_pts;
	this->type = type;

	params = new lineParams;
	pts = new gridPoint*[no_of_pts];

	inited = 1;
	is_successful = 1;
}
void gridLine::assignAddrs(gridPoint *start_addr, int offset_diff) {
	if(!inited) {
		printf("Error in gridLine::assignAddrs: Line has not been initialized\n");
		return;
	}
	for(int j = 0; j < no_of_pts; j++) {
		int addr_offset = j * offset_diff;
		pts[j] = start_addr + addr_offset;

		pts[j]->alpha[type] = (double)j / (double)(no_of_pts - 1);
		pts[j]->alpha_wt[type] = 1.0 / (double)no_of_pts;
		pts[j]->inter_alpha_diff[type] = 0;
		pts[j]->intra_alpha_diff[type] = 0;
	}
	start_pt = pts[0];
	end_pt = pts[no_of_pts - 1];
}
void gridLine::updateParams() {
	double start_x = pts[0]->x, end_x = pts[no_of_pts - 1]->x;
	double start_y = pts[0]->y, end_y = pts[no_of_pts - 1]->y;

	if(end_x == start_x) {
		params->m = params->c = INF_VAL;
		params->is_vert = 1;
	} else {
		params->m = (end_y - start_y) / (end_x - start_x);
		params->c = start_y - params->m * start_x;
		params->is_vert = 0;
	}

}

LineTrackerParams::LineTrackerParams(int _grid_size_x, int _grid_size_y,
	int _patch_size, bool _use_constant_slope, bool _use_ls,
	double _inter_alpha_thresh, double _intra_alpha_thresh,
	bool _reset_pos, bool _reset_template, bool _debug_mode){
	grid_size_x = _grid_size_x;
	grid_size_y = _grid_size_y;
	patch_size = _patch_size;
	use_constant_slope = _use_constant_slope;
	use_ls = _use_ls;
	inter_alpha_thresh = _inter_alpha_thresh;
	intra_alpha_thresh = _intra_alpha_thresh;
	reset_pos = _reset_pos;
	reset_template = _reset_template;
	debug_mode = _debug_mode;
}
LineTrackerParams::LineTrackerParams(const LineTrackerParams *params) :
grid_size_x(LINE_GRID_SIZE_X),
grid_size_y(LINE_GRID_SIZE_Y),
patch_size(LINE_PATCH_SIZE),
use_constant_slope(LINE_USE_CONSTANT_SLOPE),
use_ls(LINE_USE_LS),
inter_alpha_thresh(LINE_INTER_ALPHA_THRESH),
intra_alpha_thresh(LINE_INTRA_ALPHA_THRESH),
reset_pos(LINE_RESET_POS),
reset_template(LINE_RESET_TEMPLATE),
debug_mode(LINE_DEBUG_MODE){
	grid_size_x = params->grid_size_x;
	grid_size_y = params->grid_size_y;
	patch_size = params->patch_size;
	use_constant_slope = params->use_constant_slope;
	use_ls = params->use_ls;
	inter_alpha_thresh = params->inter_alpha_thresh;
	intra_alpha_thresh = params->intra_alpha_thresh;
	reset_pos = params->reset_pos;
	reset_template = params->reset_template;
	debug_mode = params->debug_mode;
}

LineTracker::LineTracker(const vector<TrackerBase*> trackers,
 const ParamType *line_params) : CompositeBase(trackers),
	params(line_params){
	frame_id = 0;

	if(params.grid_size_y < 0)
		params.grid_size_y = params.grid_size_x;

	name = "line";

	int grid_size = params.grid_size_x * params.grid_size_y;
	if(grid_size != n_trackers){
		printf("No. of trackers provided: %d\n", n_trackers);
		printf("No. of trackers needed for the grid: %d\n", grid_size);
		throw std::invalid_argument("LineTracker :: Mismatch between grid dimensions and no. of trackers");
	}

	corner_tracker_ids[0] = 0;
	corner_tracker_ids[1] = params.grid_size_x - 1;
	corner_tracker_ids[3] = n_trackers - params.grid_size_x;
	corner_tracker_ids[2] = n_trackers - 1;

	tracker_pos = new cv::Point2d[n_trackers];

	curr_grid_pts = new gridPoint[n_trackers];
	prev_grid_pts = new gridPoint[n_trackers];

	mean_horz_params = new lineParams(0, 0);
	mean_vert_params = new lineParams(INF_VAL, INF_VAL);

	//intersections_x = new double[n_trackers];
	//intersections_y = new double[n_trackers];

	if(params.debug_mode) {
		printf("\n initializing horizontal lines...\n");
	}
	curr_horz_lines = new gridLine[params.grid_size_y];
	prev_horz_lines = new gridLine[params.grid_size_y];
	for(int i = 0; i < params.grid_size_y; i++) {
		int start_offset = i * params.grid_size_x;
		if(params.debug_mode) {
			printf("\t calling init...\n");
		}
		curr_horz_lines[i].init(params.grid_size_x, 0);
		prev_horz_lines[i].init(params.grid_size_x, 0);
		if(params.debug_mode) {
			printf("\t calling assignAddrs...\n");
		}
		curr_horz_lines[i].assignAddrs(curr_grid_pts + start_offset, 1);
		prev_horz_lines[i].assignAddrs(prev_grid_pts + start_offset, 1);
	}

	if(params.debug_mode) {
		printf("\n initializing vertical lines...\n");
	}
	curr_vert_lines = new gridLine[params.grid_size_x];
	prev_vert_lines = new gridLine[params.grid_size_x];
	for(int i = 0; i < params.grid_size_x; i++) {
		if(params.debug_mode) {
			printf("\t calling init...\n");
		}
		curr_vert_lines[i].init(params.grid_size_y, 1);
		prev_vert_lines[i].init(params.grid_size_y, 1);
		if(params.debug_mode) {
			printf("\t calling assignAddrs...\n");
		}
		curr_vert_lines[i].assignAddrs(curr_grid_pts + i, params.grid_size_x);
		prev_vert_lines[i].assignAddrs(prev_grid_pts + i, params.grid_size_x);
	}


	is_initialized = 0;
	for(int i = 0; i < n_trackers; i++) {}

	if(params.debug_mode) {
		printf("\n done initializing\n");
	}
}

void LineTracker::initialize(const cv::Mat& cv_corners) {
	printf("Using Line Tracker with:\n\t");
	printf("grid_size_x = %d\n\t", params.grid_size_x);
	printf("grid_size_y = %d\n\t", params.grid_size_y);
	printf("use_constant_slope = %d\n\t", params.use_constant_slope);
	printf("use_ls = %d\n\t", params.use_ls);
	printf("inter_alpha_thresh = %f\n\t", params.inter_alpha_thresh);
	printf("intra_alpha_thresh = %f\n\t", params.intra_alpha_thresh);
	printf("reset_pos = %d\n\t", params.reset_pos);
	printf("reset_template = %d\n", params.reset_template);

	cv_corners_mat.create(2, 4, CV_64FC1);

	initGridPositions(cv_corners);

	if(params.use_ls) {
		updateLineParamsLS(curr_horz_lines, prev_horz_lines,
			mean_horz_params, params.grid_size_y, 0);
		updateLineParamsLS(curr_vert_lines, prev_vert_lines,
			mean_vert_params, params.grid_size_x, 1);
	} else {
		updateLineParamsWeightedLS(curr_horz_lines, prev_horz_lines, mean_horz_params, params.grid_size_y, 0);
		updateLineParamsWeightedLS(curr_vert_lines, prev_vert_lines, mean_vert_params, params.grid_size_x, 1);
	}
	cv::Mat tracker_corners(2, 4, CV_64FC1);
	for(int tracker_id = 0; tracker_id < n_trackers; tracker_id++) {
		tracker_corners.at<double>(0, 0) = tracker_corners.at<double>(0, 3) = curr_grid_pts[tracker_id].x;
		tracker_corners.at<double>(1, 0) = tracker_corners.at<double>(1, 1) = curr_grid_pts[tracker_id].y;
		tracker_corners.at<double>(0, 1) = tracker_corners.at<double>(0, 2) = curr_grid_pts[tracker_id].x + params.patch_size;
		tracker_corners.at<double>(1, 2) = tracker_corners.at<double>(1, 3) = curr_grid_pts[tracker_id].y + params.patch_size;
		trackers[tracker_id]->initialize(tracker_corners);
		tracker_pos[tracker_id].x = curr_grid_pts[tracker_id].x;
		tracker_pos[tracker_id].y = curr_grid_pts[tracker_id].y;
	}
	for(int i = 0; i < params.grid_size_x; i++) {
		curr_vert_lines[i].updateParams();
		prev_vert_lines[i].updateParams();
	}
	for(int i = 0; i < params.grid_size_y; i++) {
		curr_horz_lines[i].updateParams();
		prev_horz_lines[i].updateParams();
	}
	updateCVCorners();
	is_initialized = 1;
}

void LineTracker::initGridPositions(const cv::Mat& cv_corners) {
	Rectd best_fit_rect = mtf::utils::getBestFitRectangle<double>(cv_corners);
	double ulx = best_fit_rect.x;
	double uly = best_fit_rect.y;
	double size_x = best_fit_rect.width;
	double size_y = best_fit_rect.height;


	/*		int tracker_dist_x = size_x/grid_size_x;
			int tracker_dist_y = size_y/grid_size_y;
			int start_x = ulx + tracker_dist_x/2;
			int start_y = uly + tracker_dist_y/2;*/

	int tracker_dist_x = size_x / (params.grid_size_x - 1);
	int tracker_dist_y = size_y / (params.grid_size_x - 1);
	int start_x = ulx;
	int start_y = uly;

	for(int i = 0; i < n_trackers; i++) {
		int grid_id_x = i % params.grid_size_x;
		int grid_id_y = i / params.grid_size_x;

		prev_grid_pts[i].x = curr_grid_pts[i].x = start_x + grid_id_x * tracker_dist_x;
		prev_grid_pts[i].y = curr_grid_pts[i].y = start_y + grid_id_y * tracker_dist_y;

		curr_grid_pts[i].xx = curr_grid_pts[i].x * curr_grid_pts[i].x;
		curr_grid_pts[i].xy = curr_grid_pts[i].x * curr_grid_pts[i].y;
	}
}
void LineTracker::update() {
	frame_id++;
	if(params.debug_mode) {
		printf("\n updating LineTracker with frame: %d...\n", frame_id);
	}

	long sum_x = 0;
	long sum_y = 0;
	double wt_sum_x = 0;
	double wt_sum_y = 0;

	for(int tracker_id = 0; tracker_id < n_trackers; tracker_id++) {
		prev_grid_pts[tracker_id].x = curr_grid_pts[tracker_id].x;
		prev_grid_pts[tracker_id].y = curr_grid_pts[tracker_id].y;
		prev_grid_pts[tracker_id].xx = curr_grid_pts[tracker_id].xx;
		prev_grid_pts[tracker_id].xy = curr_grid_pts[tracker_id].xy;

		trackers[tracker_id]->update();
		const cv::Mat &tracker_corners = trackers[tracker_id]->getRegion();
		curr_grid_pts[tracker_id].x = tracker_pos[tracker_id].x = (tracker_corners.at<double>(0, 0) +
			tracker_corners.at<double>(0, 2)) / 2;
		curr_grid_pts[tracker_id].y = tracker_pos[tracker_id].y = (tracker_corners.at<double>(1, 0) +
			tracker_corners.at<double>(1, 2)) / 2;

		curr_grid_pts[tracker_id].xx = curr_grid_pts[tracker_id].x * curr_grid_pts[tracker_id].x;
		curr_grid_pts[tracker_id].xy = curr_grid_pts[tracker_id].x * curr_grid_pts[tracker_id].y;
	}
	if(params.use_ls) {
		if(params.debug_mode) {
			printf("\nUpdating vertical lines...\n");
		}
		updateLineParamsLS(curr_vert_lines, prev_vert_lines, mean_vert_params,
			params.grid_size_x, 1);
		if(params.use_constant_slope) {
			resetLineParamsToMean(curr_vert_lines, prev_vert_lines, mean_vert_params, params.grid_size_x, 0);
		}
		if(params.debug_mode) {
			printf("\nUpdating horizontal lines...\n");
		}
		updateLineParamsLS(curr_horz_lines, prev_horz_lines, mean_horz_params,
			params.grid_size_y, 0);
		if(params.use_constant_slope) {
			resetLineParamsToMean(curr_horz_lines, prev_horz_lines, mean_horz_params, params.grid_size_y, 0);
		}

	} else {
		if(params.debug_mode) {
			printf("\nUpdating vertical lines...\n");
		}
		updateAlpha(curr_vert_lines, prev_vert_lines, params.grid_size_x, 1);
		int successful_lines = updateLineParamsWeightedLS(curr_vert_lines, prev_vert_lines, mean_vert_params,
			params.grid_size_x, 1);
		if(successful_lines == 0) {
			if(params.debug_mode) {
				printf("None of the vertical lines were successfully tracked; resetting all params to their last values\n");
			}
			resetLineParamsToPrev(curr_vert_lines, prev_vert_lines, params.grid_size_x);
		} else if(successful_lines < params.grid_size_y) {
			if(params.debug_mode) {
				printf("Only %d vertical lines were successfully tracked; setting mean params for the rest...\n", successful_lines);
			}
			resetLineParamsToMean(curr_vert_lines, prev_vert_lines, mean_vert_params, params.grid_size_x, 1);
		}
		if(params.debug_mode) {
			printf("\nUpdating horizontal lines...\n");
		}
		updateAlpha(curr_horz_lines, prev_horz_lines, params.grid_size_y, 0);
		successful_lines = updateLineParamsWeightedLS(curr_horz_lines, prev_horz_lines, mean_horz_params,
			params.grid_size_y, 0);
		if(successful_lines == 0) {
			if(params.debug_mode) {
				printf("None of the horizontal lines were successfully tracked; resetting all params to their last values\n");
			}
			resetLineParamsToPrev(curr_horz_lines, prev_horz_lines, params.grid_size_y);
		} else if(successful_lines < params.grid_size_y) {
			if(params.debug_mode) {
				printf("Only %d horizontal lines were successfully tracked; setting mean params for the rest\n", successful_lines);
			}
			resetLineParamsToMean(curr_horz_lines, prev_horz_lines,
				mean_horz_params, params.grid_size_y, 1);
		}
	}
	updateGridWithLineIntersections();
	updateDistanceWeights();
	resetTrackerStates();
	updateCVCorners();
}

void LineTracker::updateAlpha(gridLine* curr_lines, gridLine* prev_lines, int no_of_lines, int line_type) {

	if(params.debug_mode) {
		printf("\n starting updateAlpha...\n");
	}
	for(int i = 0; i < no_of_lines; i++) {
		int no_of_pts = prev_lines[i].no_of_pts;
		double curr_m = prev_lines[i].params->m;
		double end_x = prev_lines[i].end_pt->x, start_x = prev_lines[i].start_pt->x;
		double end_y = prev_lines[i].end_pt->y, start_y = prev_lines[i].start_pt->y;
		double diff_x = end_x - start_x;
		double diff_y = end_y - start_y;
		int is_vert = (fabs(diff_x) <= SMALL_VAL), is_horz = (fabs(diff_y) <= SMALL_VAL);
		if(is_vert && is_horz) {
			printf("\n\nError in updateAlpha: Line %d is too small\n\n", i);
			continue;
		}
		if(params.debug_mode) {
			printf("\nLine %d: (%8.3f, %8.3f) to (%8.3f, %8.3f)\n", i, start_x, start_y, end_x, end_y);
		}
		for(int j = 0; j < no_of_pts; j++) {

			double curr_x = curr_lines[i].pts[j]->x;
			double curr_y = curr_lines[i].pts[j]->y;


			double alpha_x = 0, alpha_y = 0, curr_alpha = 0;
			double intra_alpha_diff = 0;

			if(is_vert) {
				curr_alpha = (curr_y - start_y) / (end_y - start_y);
				alpha_x = alpha_y = curr_alpha;
			} else if(is_horz) {
				curr_alpha = (curr_x - start_x) / (end_x - start_x);
				alpha_x = alpha_y = curr_alpha;
			} else {
				alpha_x = (curr_x - start_x) / (end_x - start_x);
				alpha_y = (curr_y - start_y) / (end_y - start_y);
				intra_alpha_diff = fabs(alpha_x - alpha_y);
				curr_alpha = (alpha_x + alpha_y) / 2.0;
			}

			double prev_alpha = curr_lines[i].pts[j]->alpha[line_type];
			double inter_alpha_diff = fabs(curr_alpha - prev_alpha);

			prev_lines[i].pts[j]->alpha[line_type] = prev_alpha;
			prev_lines[i].pts[j]->alpha_wt[line_type] = curr_lines[i].pts[j]->alpha_wt[line_type];
			prev_lines[i].pts[j]->intra_alpha_diff[line_type] = curr_lines[i].pts[j]->intra_alpha_diff[line_type];
			prev_lines[i].pts[j]->inter_alpha_diff[line_type] = curr_lines[i].pts[j]->inter_alpha_diff[line_type];

			if((intra_alpha_diff > params.intra_alpha_thresh) || (inter_alpha_diff > params.inter_alpha_thresh)) {
				curr_lines[i].pts[j]->alpha_wt[line_type] = 0;
			} else {
				curr_lines[i].pts[j]->alpha[line_type] = curr_alpha;
				curr_lines[i].pts[j]->alpha_wt[line_type] = 1.0 / (1.0 + inter_alpha_diff);
				curr_lines[i].pts[j]->intra_alpha_diff[line_type] = intra_alpha_diff;
				curr_lines[i].pts[j]->inter_alpha_diff[line_type] = inter_alpha_diff;
			}
			if(params.debug_mode) {
				printf("\tpt %d: (x: %8.3f y: %8.3f m: %8.5f ax: %8.5f, ay: %8.5f ca: %8.5f pa: %8.5f ied: %8.5f iad: %8.5f aw: %8.5f)\n",
					j, curr_x, curr_y, curr_m, alpha_x, alpha_y, curr_alpha, prev_alpha, inter_alpha_diff, intra_alpha_diff, curr_lines[i].pts[j]->alpha_wt[line_type]);
			}
		}

		/*if(debug_mode) {
			printf("\n\n");
			}*/
	}
	/*if(debug_mode) {
		printf("\n done updateAlpha\n");
		}*/
}

int LineTracker::updateLineParamsLS(gridLine* curr_lines, gridLine* prev_lines, lineParams *mean_params,
	int no_of_lines, int line_type) {
	if(params.debug_mode) {
		//printf("\n starting updateLineParamsLS\n");
	}
	double m_sum = 0, c_sum = 0, c_diff_sum = 0;
	int is_vert = 0;

	for(int i = 0; i < no_of_lines; i++) {

		prev_lines[i].params->m = curr_lines[i].params->m;
		prev_lines[i].params->c = curr_lines[i].params->c;
		prev_lines[i].params->c_diff = curr_lines[i].params->c_diff;

		double mean_x = 0, mean_y = 0, mean_xx = 0, mean_xy = 0;
		double wt_sum = 0;
		int successful_pts = 0;

		if(params.debug_mode) {
			printf("\nLine %d\n", i);
		}
		for(int j = 0; j < curr_lines[i].no_of_pts; j++) {

			double pt_x = curr_lines[i].pts[j]->x;
			double pt_y = curr_lines[i].pts[j]->y;
			double pt_xx = curr_lines[i].pts[j]->xx;
			double pt_xy = curr_lines[i].pts[j]->xy;

			mean_x += pt_x / curr_lines[i].no_of_pts;
			mean_y += pt_y / curr_lines[i].no_of_pts;
			mean_xy += pt_xy / curr_lines[i].no_of_pts;
			mean_xx += pt_xx / curr_lines[i].no_of_pts;

			if(params.debug_mode) {
				printf("\tpt %d: (%8.3f, %8.3f)\n",
					j, pt_x, pt_y);
			}
		}
		double mean_x2 = mean_x * mean_x;

		if(fabs(mean_xx - mean_x2) <= TINY_VAL) {
			is_vert = 1;
			curr_lines[i].params->c = curr_lines[i].params->m = INF_VAL;
		} else {
			curr_lines[i].params->m = (mean_xy - mean_x * mean_y) / (mean_xx - mean_x2);
			curr_lines[i].params->c = mean_y - mean_x * curr_lines[i].params->m;
			m_sum += curr_lines[i].params->m;
			c_sum += curr_lines[i].params->c;
		}
		c_diff_sum += curr_lines[i].params->c - prev_lines[i].params->c;
		if(params.debug_mode) {
			printf("mean_x = %12.6f mean_y = %12.6f, mean_xx = %12.6f mean_xy = %12.6f m = %12.6f c = %12.6f prev_m = %12.6f prev_c = %12.6f\n\n",
				mean_x, mean_y, mean_xx, mean_xy, curr_lines[i].params->m, curr_lines[i].params->c,
				prev_lines[i].params->m, prev_lines[i].params->c);
		}
		curr_lines[i].is_successful = 1;
	}
	if(is_vert) {
		mean_params->m = mean_params->c = INF_VAL;
	} else {
		mean_params->c = c_sum / no_of_lines;
		mean_params->m = m_sum / no_of_lines;
	}
	mean_params->c_diff = c_diff_sum / no_of_lines;
	if(params.debug_mode) {
		printf("mean_m = %12.6f mean_c = %12.6f, mean_cdiff = %12.6f\n\n",
			mean_params->m, mean_params->c, mean_params->c_diff);
	}
	return no_of_lines;
}

int LineTracker::updateLineParamsWeightedLS(gridLine* curr_lines, gridLine* prev_lines, lineParams *mean_params,
	int no_of_lines, int line_type) {
	if(params.debug_mode) {
		printf("starting updateLineParamsWeightedLS\n");
	}

	double theta_sum = 0.0;
	int successful_lines = 0;
	double m_sum = 0, c_sum = 0, c_diff_sum = 0;
	int is_vert = 0;

	for(int i = 0; i < no_of_lines; i++) {


		prev_lines[i].params->m = curr_lines[i].params->m;
		prev_lines[i].params->c = curr_lines[i].params->c;
		prev_lines[i].params->c_diff = curr_lines[i].params->c_diff;

		double mean_x = 0, mean_y = 0, mean_xx = 0, mean_xy = 0;
		double wt_sum = 0;
		//double theta = 0.0;
		int successful_pts = 0;
		if(params.debug_mode) {
			printf("\nLine %d\n", i);
		}
		for(int j = 0; j < curr_lines[i].no_of_pts; j++) {


			double pt_x = curr_lines[i].pts[j]->x;
			double pt_y = curr_lines[i].pts[j]->y;
			/*double pt_xx = curr_lines[i].pts[j]->xx;
			double pt_xy = curr_lines[i].pts[j]->xy;*/

			double dist_wt = curr_lines[i].pts[j]->wt;
			double alpha_wt = curr_lines[i].pts[j]->alpha_wt[line_type];
			double alpha = curr_lines[i].pts[j]->alpha[line_type];
			double inter_alpha_diff = curr_lines[i].pts[j]->inter_alpha_diff[line_type];
			double intra_alpha_diff = curr_lines[i].pts[j]->intra_alpha_diff[line_type];

			double net_wt = dist_wt * alpha_wt;
			if(net_wt > 0) {
				curr_lines[i].pts[j]->is_successful[line_type] = 1;
				successful_pts++;
			} else {
				curr_lines[i].pts[j]->is_successful[line_type] = 0;
			}

			mean_x += net_wt * pt_x;
			mean_y += net_wt * pt_y;


			/*mean_xy += net_wt*pt_xy;
			mean_xx += net_wt*pt_xx;*/
			wt_sum += net_wt;

			if(params.debug_mode) {
				printf("\tpt %d: (x: %8.3f, y: %8.3f, a: %8.5f, ie: %8.5f, ia: %8.5f, dw: %8.5f, aw: %8.5f, nw: %8.5f)\n",
					j, pt_x, pt_y, alpha, inter_alpha_diff, intra_alpha_diff, dist_wt, alpha_wt, net_wt);
			}
		}
		if(params.debug_mode) {
			printf("Successfully tracked pts %d\n", successful_pts);
		}
		if(successful_pts < 2) {
			curr_lines[i].is_successful = 0;
			/*if (debug_mode){
				printf("Insufficient no. of successfully tracked pts %d for line %d\n",successful_pts, i);
				}*/
		} else {
			mean_x /= wt_sum;
			mean_y /= wt_sum;
			double numer = 0, denom = 0;
			double diff_x_mean = 0, diff_y_mean = 0;
			for(int j = 0; j < curr_lines[i].no_of_pts; j++) {
				double pt_x = curr_lines[i].pts[j]->x;
				double pt_y = curr_lines[i].pts[j]->y;
				double diff_x = pt_x - mean_x;
				double diff_y = pt_y - mean_y;
				numer += diff_x * diff_y;
				denom += diff_x * diff_x;
				diff_x_mean += fabs(diff_x);
				diff_y_mean += fabs(diff_y);
			}
			diff_x_mean /= curr_lines[i].no_of_pts;
			diff_y_mean /= curr_lines[i].no_of_pts;

			/*mean_xy /= wt_sum;
			mean_xx /= wt_sum;*/
			/*double mean_x2 = mean_x*mean_x;*/
			//curr_lines[i].theta = atan2(mean_xy-mean_x*mean_y, mean_xx-mean_x2);

			if((denom <= TINY_VAL) || (diff_x_mean <= SMALL_VAL) || ((denom <= SMALL_VAL) && (numer <= SMALL_VAL))) {
				is_vert = 1;
				curr_lines[i].params->is_vert = 1;
				curr_lines[i].params->c = curr_lines[i].params->m = INF_VAL;
			} else {
				curr_lines[i].params->m = numer / denom;
				curr_lines[i].params->c = mean_y - mean_x * curr_lines[i].params->m;
				m_sum += curr_lines[i].params->m;
				c_sum += curr_lines[i].params->c;
			}

			curr_lines[i].params->c_diff = curr_lines[i].params->c - prev_lines[i].params->c;
			c_diff_sum += curr_lines[i].params->c_diff;
			curr_lines[i].is_successful = 1;
			successful_lines++;
			if(params.debug_mode) {
				printf("mean_x: %12.6f mean_y: %12.6f, numer: %12.6f denom: %12.6f wt_sum: %12.6f dxm: %12.6f dym: %12.6f m: %12.6f c: %12.6f cdiff: %12.6f vert: %d\n\n",
					mean_x, mean_y, numer, denom, wt_sum, diff_x_mean, diff_y_mean,
					curr_lines[i].params->m, curr_lines[i].params->c, curr_lines[i].params->c_diff, curr_lines[i].params->is_vert);
			}
		}
	}
	if(is_vert) {
		if(mean_params->c == INF_VAL){
			mean_params->c_diff = 0;
		} else{
			mean_params->c_diff = INF_VAL;
		}
		mean_params->m = mean_params->c = INF_VAL;

	} else if(successful_lines > 0) {
		mean_params->c = c_sum / successful_lines;
		mean_params->m = m_sum / successful_lines;
		mean_params->c_diff = c_diff_sum / successful_lines;
	} else{
		if(params.debug_mode) {
			printf("no lines were successful\n");
		}
	}
	if(params.debug_mode) {
		printf("successful_lines: %d mean_m: %12.6f mean_c: %12.6f mean_c_diff: %12.6f\n", successful_lines, mean_params->m, mean_params->c, mean_params->c_diff);
		//printf("done updateLineParamsWeightedLS\n");
	}
	/*double mean_theta = theta_sum/(double)no_of_lines;
	double mean_m = tan(mean_theta);
	for(int i = 0;i<no_of_lines;i++){
	lines[i].m = mean_m;
	}*/
	return successful_lines;
}

void LineTracker::resetLineParamsToPrev(gridLine* curr_lines, gridLine* prev_lines, int no_of_lines) {
	for(int i = 0; i < no_of_lines; i++) {
		if(curr_lines[i].is_successful) {
			continue;
		}
		curr_lines[i].params->m = prev_lines[i].params->m;
		curr_lines[i].params->c = prev_lines[i].params->c + prev_lines[i].params->c_diff;
		curr_lines[i].params->c_diff = prev_lines[i].params->c_diff;
	}
}

void LineTracker::resetLineParamsToMean(gridLine* curr_lines, gridLine* prev_lines, 
	lineParams* mean_params, int no_of_lines, int reset_c) {
	if(params.debug_mode) {
		printf("resetting line params to mean\n");
	}
	for(int i = 0; i < no_of_lines; i++) {
		curr_lines[i].params->m = mean_params->m;
		if(params.debug_mode) {
			printf("\tline %d :: m = %12.6f\n", i, curr_lines[i].params->m);
		}
		if(reset_c) {
			curr_lines[i].params->c = prev_lines[i].params->c + mean_params->c_diff;
			curr_lines[i].params->c_diff = mean_params->c_diff;
		}
	}
}

void LineTracker::updateGridWithLineIntersections() {
	for(int i = 0; i < n_trackers; i++) {
		double inter_pt_x = 0, inter_pt_y = 0;

		int vert_id = i % params.grid_size_x;
		int horz_id = i / params.grid_size_x;

		double vert_m = curr_vert_lines[vert_id].params->m;
		double horz_m = curr_horz_lines[horz_id].params->m;

		double vert_c = curr_vert_lines[vert_id].params->c;
		double horz_c = curr_horz_lines[horz_id].params->c;

		if(vert_m == horz_m) {
			printf("Error in updateLineIntersections:: slopes of vertical line %d and horz line %d are identical\n", vert_id, horz_id);
			exit(0);
		} else if(vert_m == INF_VAL) {
			inter_pt_x = curr_vert_lines[vert_id].pts[0]->x;
			inter_pt_y = horz_m * inter_pt_x + horz_c;
		} else if(horz_m == INF_VAL) {
			inter_pt_x = curr_horz_lines[horz_id].pts[0]->x;
			inter_pt_y = vert_m * inter_pt_x + vert_c;
		} else if(vert_m == 0) {
			inter_pt_x = (horz_c - vert_c) / (-horz_m);
			inter_pt_y = curr_vert_lines[vert_id].pts[0]->y;
		} else if(horz_m == 0) {
			inter_pt_x = (horz_c - vert_c) / (vert_m);
			inter_pt_y = curr_horz_lines[horz_id].pts[0]->y;
		} else {
			inter_pt_x = (horz_c - vert_c) / (vert_m - horz_m);
			inter_pt_y = vert_m * inter_pt_x + vert_c;
		}
		curr_grid_pts[i].x = inter_pt_x;
		curr_grid_pts[i].y = inter_pt_y;
		curr_grid_pts[i].xx = inter_pt_x * inter_pt_x;
		curr_grid_pts[i].xy = inter_pt_x * inter_pt_y;
	}
}

void LineTracker::updateDistanceWeights() {

	double wt_sum = 0.0, wt_sum_x = 0.0, wt_sum_y = 0.0;
	for(int i = 0; i < n_trackers; i++) {

		prev_grid_pts[i].wt = curr_grid_pts[i].wt;
		prev_grid_pts[i].wt_x = curr_grid_pts[i].wt_x;
		prev_grid_pts[i].wt_y = curr_grid_pts[i].wt_y;

		double dist_x = fabs(curr_grid_pts[i].x - tracker_pos[i].x);
		double dist_y = fabs(curr_grid_pts[i].y - tracker_pos[i].y);

		curr_grid_pts[i].wt = 1.0 / (1.0 + dist_x + dist_y);
		curr_grid_pts[i].wt_x = 1.0 / (1.0 + dist_x);
		curr_grid_pts[i].wt_y = 1.0 / (1.0 + dist_y);

		wt_sum += curr_grid_pts[i].wt;
		wt_sum_x += curr_grid_pts[i].wt_x;
		wt_sum_y += curr_grid_pts[i].wt_y;
	}
	for(int i = 0; i < n_trackers; i++) {
		curr_grid_pts[i].wt /= wt_sum;
		curr_grid_pts[i].wt_x /= wt_sum_x;
		curr_grid_pts[i].wt_y /= wt_sum_y;
		//printf("Tracker %d wt: %f\n", i, dist_wts[i]);
	}
}

void LineTracker::resetTrackerStates() {
	cv::Mat tracker_corners(2, 4, CV_64FC1);
	for(int tracker_id = 0; tracker_id < n_trackers; tracker_id++) {
		tracker_corners.at<double>(0, 0) = tracker_corners.at<double>(0, 3) = curr_grid_pts[tracker_id].x;
		tracker_corners.at<double>(1, 0) = tracker_corners.at<double>(1, 1) = curr_grid_pts[tracker_id].y;
		tracker_corners.at<double>(0, 1) = tracker_corners.at<double>(0, 2) = curr_grid_pts[tracker_id].x + params.patch_size;
		tracker_corners.at<double>(1, 2) = tracker_corners.at<double>(1, 3) = curr_grid_pts[tracker_id].y + params.patch_size;

		if(params.reset_template) {
			trackers[tracker_id]->initialize(tracker_corners);
		} else if(params.reset_pos) {
			trackers[tracker_id]->setRegion(tracker_corners);
		}
		//tracker_pos[i].x = curr_grid_pts[i].x;
		//tracker_pos[i].y = curr_grid_pts[i].y;
	}
}
void LineTracker::updateCVCorners() {
	for(int corner_id = 0; corner_id < 4; corner_id++) {
		cv_corners_mat.at<double>(0, corner_id) = tracker_pos[corner_tracker_ids[corner_id]].x;
		cv_corners_mat.at<double>(1, corner_id) = tracker_pos[corner_tracker_ids[corner_id]].y;
	}
}
_MTF_END_NAMESPACE
