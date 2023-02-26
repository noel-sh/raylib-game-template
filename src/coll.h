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

// Represents an AABB specified by centre location and half width/height.
typedef struct coll_aabb_t
{
	float x;
	float y;
	float half_w;
	float half_h;
} coll_aabb_t;

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

// sweep an AABB through a grid and return the closest hit
int coll_sweep_aabb_grid(coll_grid_t grid, coll_aabb_t aabb, float ray_x, float ray_y, coll_trace_hit_t* out_hit);

#if defined(__cplusplus)
}
#endif
