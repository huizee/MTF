#ifndef MTF_PF_PARAMS_H
#define MTF_PF_PARAMS_H

#include "mtf/Macros/common.h"

#define PF_MAX_ITERS 10
#define PF_N_PARTICLES 200
#define PF_EPSILON 0.01
#define PF_DYN_MODEL 0
#define PF_UPD_TYPE 0
#define PF_LIKELIHOOD_FUNC 0
#define PF_RESAMPLING_TYPE 0
#define PF_RESET_TO_MEAN false
#define PF_MEAN_TYPE 1
#define PF_UPDATE_SAMPLER_WTS 0
#define PF_MIN_DISTR_WT 0.5
#define PF_ADAPTIVE_RESAMPLING_THRESH 0
#define PF_CORNER_SIGMA_D 0.06
#define PF_PIX_SIGMA 0.04
#define PF_MEASUREMENT_SIGMA 0.1
#define PF_SHOW_PARTICLES 0
#define PF_UPDATE_TEMPLATE 0
#define PF_JACOBIAN_AS_SIGMA false
#define PF_DEBUG_MODE false

_MTF_BEGIN_NAMESPACE

struct PFParams{
	// supported dynamic models for sample generation
	enum class DynamicModel{
		RandomWalk,
		AutoRegression1
	};
	enum class UpdateType{
		Additive,
		Compositional
	};
	enum class ResamplingType{
		None,
		BinaryMultinomial,
		LinearMultinomial,
		Residual
	};
	enum class LikelihoodFunc{
		AM,
		Gaussian,
		Reciprocal
	};
	enum class MeanType{
		None,
		SSM,
		Corners
	};
	static const char* toString(DynamicModel _dyn_model);
	static const char* toString(UpdateType _upd_type);
	static const char* toString(ResamplingType _resampling_type);
	static const char* toString(LikelihoodFunc _likelihood_func);
	static const char* toString(MeanType _likelihood_func);
	//! maximum iterations of the PF algorithm to run for each frame
	int max_iters;
	//! number of particles to use
	int n_particles;
	//! iterations will be terminated when L2 norm of the change in tracker corners exceeds this
	double epsilon;
	DynamicModel dynamic_model;
	UpdateType update_type;
	LikelihoodFunc likelihood_func;
	ResamplingType resampling_type;
	/**
	method used for computing the mean of the SSM states corresponding to the particles.\
	0: No mean computed - just use the state of the particle with the highest weight
	1: let the SSM compute the mean of the samples
	2: mean of the corners of the bounding boxes corresponding to the particles
	*/
	MeanType mean_type;
	//! reset all particles to the mean/optimal corners found in each frame
	bool reset_to_mean;
	/**
	standarsd deviations of the Gaussian distributions to use for the samplers
	*/
	vectorvd ssm_sigma;
	/**
	mean of the Gaussian distributions to use for the samplers
	*/
	vectorvd ssm_mean;
	/**
	update the proportion of samples taken from different sampler according to the
	weights	of the samples generated by each
	*/
	bool update_distr_wts;
	/**
	fraction of the total particles that will always be evenly
	distributed between the samplers;
	*/
	double min_distr_wt;
	/**
	maximum ratio between the number of effective particles and the
	total particles for resampling to be performed;
	setting it to <=0 or >1 disables adaptive resampling 
	*/
	double adaptive_resampling_thresh;
	vectord pix_sigma;
	double measurement_sigma;
	int show_particles;
	bool enable_learning;
	bool jacobian_as_sigma;
	//! decides whether logging data will be printed for debugging purposes; 
	bool debug_mode;
	PFParams(int _max_iters, int _n_particles, double _epsilon,
		DynamicModel _dyn_model, UpdateType _upd_type,
		LikelihoodFunc _likelihood_func,
		ResamplingType _resampling_type,
		MeanType _mean_type, bool _reset_to_mean,
		const vectorvd &_ssm_sigma, const vectorvd &_ssm_mean,
		bool _update_distr_wts, double _min_distr_wt,
		double _adaptive_resampling_thresh,
		const vectord &_pix_sigma, double _measurement_sigma,
		int _show_particles, bool _enable_learning,
		bool _jacobian_as_sigma, bool _debug_mode);
	PFParams(const PFParams *params = nullptr);
	/**
	parse the provided mean and sigma and apply several priors
	to get the final parameters for all distributions
	*/
	bool processDistributions(vector<VectorXd> &state_sigma,
		vector<VectorXd> &state_mean, VectorXi &distr_n_samples,
		int &n_distr, int ssm_state_size);
};

_MTF_END_NAMESPACE

#endif

