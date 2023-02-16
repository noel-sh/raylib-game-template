// A simple C API for 2d collision


typedef struct coll_grid_t
{
	float offset_x;
	float offset_y;
	float cell_size;
	int width;
	int height;
	void* context;
	// user provided callback which returns true if the cell should be considered a hit
	int (*cb_has_hit)(void* ctx, int x, int y);
} coll_grid_t;

typedef struct coll_ray_t
{
	float start_x;
	float start_y;
	float end_x;
	float end_y;
} coll_ray_t;

typedef struct coll_trace_hit_t
{
	float hit_pos_x;
	float hit_pos_y;
	float hit_normal_x;
	float hit_normal_y;
	float dist;
	int hit_value;
} coll_trace_hit_t;


#if defined(__cplusplus)
extern "C" {
#endif

// raycast a line segment through a grid and return the closest hit
int coll_ray_grid(coll_grid_t grid, coll_ray_t ray, coll_trace_hit_t* out_hit);


#if defined(__cplusplus)
}
#endif
