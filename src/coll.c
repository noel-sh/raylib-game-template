#include "coll.h"

#include <math.h>
#include <float.h>

static float coll_line_length(float x1, float y1, float x2, float y2)
{
	return sqrtf(((x1 - x2) * (x1 - x2)) + ((y1 - y2) * (y1 - y2)));
}


static int check_ray_grid_bounds(coll_grid_t grid, coll_ray_t ray)
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


int coll_ray_grid(coll_grid_t grid, coll_ray_t ray, coll_trace_hit_t* out_hit)
{
	// check if ray bounds intersects grid bounds
	if (check_ray_grid_bounds(grid, ray) == 0) return 0;

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

