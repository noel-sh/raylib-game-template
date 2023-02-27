#include "coll.h"

#include <math.h>
#include <float.h>

static float coll_line_length(float x1, float y1, float x2, float y2)
{
	return sqrtf(((x1 - x2) * (x1 - x2)) + ((y1 - y2) * (y1 - y2)));
}


static int check_rect_grid_bounds(coll_grid_t grid, coll_ray_t ray)
{
	float rec1x = grid.offset_x;
	float rec1y = grid.offset_y;
	float rec1w = (grid.cell_size * grid.width);
	float rec1h = (grid.cell_size * grid.height);

	float rec2x = ray.start_x < ray.end_x ? ray.start_x : ray.end_x;
	float rec2y = ray.start_y < ray.end_y ? ray.start_y : ray.end_y;
	float rec2w = fabsf(ray.end_x - ray.start_x);
	float rec2h = fabsf(ray.end_x - ray.start_x);

	if ((rec1x < (rec2x + rec2w) && (rec1x + rec1w) > rec2x) &&
		(rec1y < (rec2y + rec2h) && (rec1y + rec1h) > rec2y)) return 1;
	return 0;
}


static int check_aabb_grid_bounds(coll_grid_t grid, coll_aabb_t aabb)
{
	float rec1x = grid.offset_x;
	float rec1y = grid.offset_y;
	float rec1w = (grid.cell_size * grid.width);
	float rec1h = (grid.cell_size * grid.height);

	float rec2x = aabb.x - aabb.half_w;
	float rec2y = aabb.y - aabb.half_h;
	float rec2w = aabb.x + aabb.half_w;
	float rec2h = aabb.y - aabb.half_h;

	if ((rec1x < (rec2x + rec2w) && (rec1x + rec1w) > rec2x) &&
		(rec1y < (rec2y + rec2h) && (rec1y + rec1h) > rec2y)) return 1;
	return 0;
}


static coll_aabb_t expand_aabb_by_aabb(coll_aabb_t a, coll_aabb_t b)
{
	float rect_l = fminf((a.x - a.half_w), (b.x - b.half_w));
	float rect_r = fmaxf((a.x + a.half_w), (b.x + b.half_w));
	float rect_t = fminf((a.y - a.half_h), (b.y - b.half_h));
	float rect_b = fmaxf((a.y + a.half_h), (b.y + b.half_h));

	float half_w = (rect_r - rect_l) / 2.0f;
	float half_h = (rect_b - rect_t) / 2.0f;

	coll_aabb_t ret = {
		rect_l + half_w,
		rect_t + half_h,
		half_w,
		half_h
	};
	return ret;
}


int coll_ray_grid(coll_grid_t grid, coll_ray_t ray, coll_trace_hit_t* out_hit)
{
	// check if ray bounds intersects grid bounds
	if (check_rect_grid_bounds(grid, ray) == 0) return 0;

	// possible hit so proceed with the raycast
	coll_trace_hit_t result = {
		.dist = FLT_MAX
	};

	float ray_length = coll_line_length(ray.start_x, ray.start_y, ray.end_x, ray.end_y);

	// transform world space ray into cell relative
	float ray_start_x = (ray.start_x - grid.offset_x) / grid.cell_size;
	float ray_start_y = (ray.start_y - grid.offset_y) / grid.cell_size;
	float ray_end_x = (ray.end_x - grid.offset_x) / grid.cell_size;
	float ray_end_y = (ray.end_y - grid.offset_y) / grid.cell_size;

	float dx = ray_end_x - ray_start_x;
	float dy = ray_end_y - ray_start_y;
	float dt_dx = 1.0f / dx;
	float dt_dy = 1.0f / dy;

	int x = (int)floorf(ray_start_x);
	int y = (int)floorf(ray_start_y);
	int n = 1;

	int incX, incY;
	float nextX, nextY;
	float t = 0.0f;
	int lastMoveWasHorizontal = 1;

	if (dx == 0.0f)
	{
		incX = 0;
		nextX = dt_dx;	// infinity
	}
	else if (ray_end_x > ray_start_x)
	{
		incX = 1;
		n += (int)floorf(ray_end_x) - x;
		nextX = (floorf(ray_start_x) + 1 - ray_start_x) * dt_dx;
	}
	else
	{
		incX = -1;
		n += x - (int)floorf(ray_end_x);
		nextX = (ray_start_x - floorf(ray_start_x)) * dt_dx;
	}

	if (dy == 0.0f)
	{
		incY = 0;
		nextY = dt_dy;	// infinity
	}
	else if (ray_end_y > ray_start_y)
	{
		incY = 1;
		n += (int)floorf(ray_end_y) - y;
		nextY = (floorf(ray_start_y) + 1.0f - ray_start_y) * dt_dy;
	}
	else
	{
		incY = -1;
		n += y - (int)floorf(ray_end_y);
		nextY = (ray_start_y - floorf(ray_start_y)) * dt_dy;
	}

	for (; n > 0; --n)
	{
		// ray might originate or terminate outside of the bounds so only check grid cell for valid cells.
		// This could be improved by fast forwarding to the first cell in bounds, and terminating early as 
		// soon as the ray leaves the bounds.
		if (x >= 0 && x < grid.width && y >= 0 && y < grid.height)
		{
			int hit = grid.cb_has_hit(grid.context, x, y);
			if (hit != 0)
			{
				// TODO handle if the ray starts intersecting... could either return that case
				// or scan forward for the next change of value? Maybe both.

				// We have a collision! Store if it is the closest collision.
				float dist = t * ray_length;
				if (dist < result.dist)
				{
					result.dist = dist;
					result.hit_value = hit;
					result.hit_pos_x = (ray_start_x + dx * t) * grid.cell_size + grid.offset_x;
					result.hit_pos_y = (ray_start_y + dy * t) * grid.cell_size + grid.offset_y;

					// calculate the surface normal from the direction we last stepped in
					if (lastMoveWasHorizontal) {
						result.hit_normal_x = (nextX < 0.0f) ? 1.0f : -1.0f;
					} else {
						result.hit_normal_y = (nextY < 0.0f) ? 1.0f : -1.0f;
					}
				}

				// we traced from start, so the first hit is the closest and we can terminate now
				goto trace_done;
			}
		}

		if (fabsf(nextY) < fabsf(nextX))
		{
			y += incY;
			t = fabsf(nextY);
			nextY += dt_dy;
			lastMoveWasHorizontal = 0;
		}
		else
		{
			x += incX;
			t = fabsf(nextX);
			nextX += dt_dx;
			lastMoveWasHorizontal = 1;
		}
	}

trace_done:
	*out_hit = result;
	return result.hit_value;
}




int coll_sweep_aabb_grid(coll_grid_t grid, coll_aabb_t aabb, float ray_x, float ray_y, coll_trace_hit_t* out_hit)
{
	// advance the ray to start on the leading edge of the aabb
	// then walk the grid following the ray from start to end
	// at each cell boundary check the entire relevant edge for a collision

	// compute aabb encompassing the source aabb from the start to end location
	coll_aabb_t sweepAABB = expand_aabb_by_aabb(aabb, (coll_aabb_t){aabb.x + ray_x, aabb.y + ray_y, aabb.half_w, aabb.half_h});
	if (check_aabb_grid_bounds(grid, sweepAABB) == 0) return 0;

	// possible hit so proceed with the raycast
	coll_trace_hit_t result = {
		.dist = FLT_MAX
	};

	coll_ray_t ray = {aabb.x, aabb.y, aabb.x + ray_x, aabb.y + ray_y};

	float ray_offset_x, ray_offset_y;

	// offset ray so it casts from the leading edge
	if (fabsf(ray_x) > fabsf(ray_y))
	{
		ray_offset_x = copysignf(aabb.half_w, ray_x);
		ray_offset_y = copysignf(aabb.half_h * (ray_y / ray_x), ray_y);
	}
	else
	{
		ray_offset_x = copysignf(aabb.half_w * (ray_x / ray_y), ray_x);
		ray_offset_y = copysignf(aabb.half_h, ray_y);
	}

	ray.start_y += ray_offset_y;
	ray.end_y += ray_offset_y;
	ray.start_x += ray_offset_x;
	ray.end_x += ray_offset_x;

	float ray_length = coll_line_length(0.0f, 0.0f, ray_x, ray_y);

	// transform world space ray into cell relative
	float ray_start_x = (ray.start_x - grid.offset_x) / grid.cell_size;
	float ray_start_y = (ray.start_y - grid.offset_y) / grid.cell_size;
	float ray_end_x = (ray.end_x - grid.offset_x) / grid.cell_size;
	float ray_end_y = (ray.end_y - grid.offset_y) / grid.cell_size;

	float dx = ray_end_x - ray_start_x;
	float dy = ray_end_y - ray_start_y;
	float dt_dx = 1.0f / dx;
	float dt_dy = 1.0f / dy;

	int x = (int)floorf(ray_start_x);
	int y = (int)floorf(ray_start_y);
	int n = 1;

	int incX, incY;
	float nextX, nextY;
	float t = 0.0f;
	int lastMoveWasHorizontal = 1;

	if (dx == 0.0f)
	{
		incX = 0;
		nextX = dt_dx;	// infinity
	}
	else if (ray_end_x > ray_start_x)
	{
		incX = 1;
		n += (int)floorf(ray_end_x) - x;
		nextX = (floorf(ray_start_x) + 1 - ray_start_x) * dt_dx;
	}
	else
	{
		incX = -1;
		n += x - (int)floorf(ray_end_x);
		nextX = (ray_start_x - floorf(ray_start_x)) * dt_dx;
	}

	if (dy == 0.0f)
	{
		incY = 0;
		nextY = dt_dy;	// infinity
	}
	else if (ray_end_y > ray_start_y)
	{
		incY = 1;
		n += (int)floorf(ray_end_y) - y;
		nextY = (floorf(ray_start_y) + 1.0f - ray_start_y) * dt_dy;
	}
	else
	{
		incY = -1;
		n += y - (int)floorf(ray_end_y);
		nextY = (ray_start_y - floorf(ray_start_y)) * dt_dy;
	}

	for (; n > 0; --n)
	{
		// TODO: Check entire edge of AABB against grid, not just the centre cell


		// ray might originate or terminate outside of the bounds so only check grid cell for valid cells.
		// This could be improved by fast forwarding to the first cell in bounds, and terminating early as 
		// soon as the ray leaves the bounds.
		if (x >= 0 && x < grid.width && y >= 0 && y < grid.height)
		{
			int hit = grid.cb_has_hit(grid.context, x, y);
			if (hit != 0)
			{
				// We have a collision! Store if it is the closest collision.
				float dist = t * ray_length;
				if (dist < result.dist)
				{
					result.dist = dist;
					result.hit_value = hit;

					result.hit_pos_x = (ray_start_x + dx * t) * grid.cell_size + grid.offset_x - ray_offset_x;
					result.hit_pos_y = (ray_start_y + dy * t) * grid.cell_size + grid.offset_y - ray_offset_y;

					// calculate the surface normal from the direction we last stepped in
					if (lastMoveWasHorizontal) {
						result.hit_normal_x = (nextX < 0.0f) ? 1.0f : -1.0f;
					}
					else {
						result.hit_normal_y = (nextY < 0.0f) ? 1.0f : -1.0f;
					}
				}

				// we traced from start, so the first hit is the closest and we can terminate now
				goto trace_done;
			}
		}

		if (fabsf(nextY) < fabsf(nextX))
		{
			y += incY;
			t = fabsf(nextY);
			nextY += dt_dy;
			lastMoveWasHorizontal = 0;
		}
		else
		{
			x += incX;
			t = fabsf(nextX);
			nextX += dt_dx;
			lastMoveWasHorizontal = 1;
		}
	}

trace_done:
	*out_hit = result;
	return result.hit_value; 
}





int coll_sweep_circle_grid(coll_grid_t grid, coll_ray_t ray, float radius, coll_trace_hit_t* out_hit)
{
	// how to do this?
	// cast multiple rays
	// narrow down the likely cells
	// check circle against aabbs, or specifically the 'leading' 2 edges of the AABB as line segments

	// STATIC circle to static line collision
	// find closest point on line and compare distance to radius

	// MOVING circle to static line
	// https://ericleong.me/research/circle-line/
	// ??? complicated!

	// maybe approximate/brute force is ok

	return 0;
}

