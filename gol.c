/* Conway's Game of Life
 *
 * Uses Simple DirectMedia Layer (SDL) 2.0 for the output.
 */
#include <stdlib.h>
#include <time.h>
#include "SDL.h"

/* Cell grid. */
struct grid
{
	/* Cells: live = 1, dead = 0 */
	char* cells;
	/* grid dimensions */
	int cols;
	int rows;
	/* implementation details */
	char* data;
	int data_index;
};

struct grid* grid_create();
void         grid_destroy(struct grid*);
int          grid_tick(struct grid*);
int          grid_draw(const struct grid*, SDL_Renderer*);

#define WINDOW_WIDTH  1024
#define WINDOW_HEIGHT 768
#define DELAY         50

int main(int argc, char** argv)
{
    SDL_Renderer* renderer = NULL;
	SDL_Window* window = NULL;
	struct grid* grid = NULL;
	int ret = 0;

	(void)argc;
	(void)argv;

	if (SDL_Init(SDL_INIT_VIDEO)
		|| SDL_CreateWindowAndRenderer(WINDOW_WIDTH, WINDOW_HEIGHT, 0, &window, &renderer)
		|| !(grid = grid_create()))
	{
		ret = 1;
		goto exit_main;
	}

    for (;;)
    {
		SDL_Event event;
event_loop:
		if (SDL_PollEvent(&event))
		{
			if (event.type == SDL_QUIT)
				break;
			goto event_loop;
		}

		grid_tick(grid);
		if ((ret = grid_draw(grid, renderer)))
			break;

		SDL_Delay(DELAY);
    }

exit_main:
	if (grid)
		grid_destroy(grid);
	if (renderer)
		SDL_DestroyRenderer(renderer);
	if (window)
		SDL_DestroyWindow(window);
    SDL_Quit();
    return ret;
}

struct grid* grid_create()
{
#define ROWS 128
#define COLS 128

#ifndef RANDOM_INIT
	/* Gosper's glider gun */
	const int gosper_glider[36] =
	{
		1 * COLS + 25,
		2 * COLS + 23, 2 * COLS + 25,
		3 * COLS + 13, 3 * COLS + 14, 3 * COLS + 21, 3 * COLS + 22, 3 * COLS + 35, 3 * COLS + 36,
		4 * COLS + 12, 4 * COLS + 16, 4 * COLS + 21, 4 * COLS + 22, 4 * COLS + 35, 4 * COLS + 36,
		5 * COLS +  1, 5 * COLS +  2, 5 * COLS + 11, 5 * COLS + 17, 5 * COLS + 21, 5 * COLS + 22,
		6 * COLS +  1, 6 * COLS +  2, 6 * COLS + 11, 6 * COLS + 15, 6 * COLS + 17, 6 * COLS + 18, 6 * COLS + 23, 6 * COLS + 25,
		7 * COLS + 11, 7 * COLS + 17, 7 * COLS + 25,
		8 * COLS + 12, 8 * COLS + 16,
		9 * COLS + 13, 9 * COLS + 14
	};
#endif

	struct grid* grid = NULL;
	int i = 0;

	if (!(grid = (struct grid* ) malloc(sizeof(struct grid))))
		return NULL;

	if (!(grid->data = (char* ) calloc(2, (size_t) ROWS * COLS)))
	{
		free(grid);
		return NULL;
	}

#ifndef RANDOM_INIT
	for (i = 0; i < (int) (sizeof(gosper_glider)/sizeof(*gosper_glider)); i++)
		grid->data[gosper_glider[i]] = 1;
#else
	srand((unsigned int)time(NULL));
	for (i = 0; i < ROWS * COLS; i++)
		grid->data[i] = rand() & 1;
#endif

	grid->rows       = ROWS;
	grid->cols       = COLS;
	grid->cells      = grid->data;
	grid->data_index = 0;
	return grid;
}

void grid_destroy(struct grid* grid)
{
	if (grid)
	{
		free(grid->data);
		free(grid);
	}
}

int grid_tick(struct grid* grid)
{
	int x = 0, y = 0;
	char* nextgen = NULL;

	if (!grid)
		return 1;

	grid->data_index = ~grid->data_index & 1;
	nextgen = &grid->data[grid->data_index * grid->rows * grid->cols];

	for (y = 0; y < grid->rows; y++)
	{
		for (x = 0; x < grid->cols; x++)
		{
			int live = 0;
			int cell = x + y * grid->cols;

			/* Count adjacent live cells. Cells outside grid are considered dead. */
			live += x ? grid->cells[cell - 1] : 0;
			live += x + 1 < grid->cols ? grid->cells[cell + 1] : 0;
			live += y ? grid->cells[cell - grid->cols] : 0;
			live += y + 1 < grid->rows ? grid->cells[cell + grid->cols] : 0;
			live += x && y ? grid->cells[cell - grid->cols - 1] : 0;
			live += x + 1 < grid->cols && y ? grid->cells[cell - grid->cols + 1] : 0;
			live += x && y + 1 < grid->rows ? grid->cells[cell + grid->cols - 1] : 0;
			live += x + 1 < grid->cols && y + 1 < grid->rows ? grid->cells[cell + grid->cols + 1] : 0;

			/* Live cell survives if exactly two or three neighbour cells are live.
			 * Dead cell becomes live cell if exactly three neighbours are live.
			 * In any other case the cell is dead in the next generation. */
			if (grid->cells[cell])
				nextgen[cell] = (live == 3 || live == 2) ? 1 : 0;
			else
				nextgen[cell] = (live == 3) ? 1 : 0;
		}
	}

	grid->cells = nextgen;
	return 0;
}

int grid_draw(const struct grid* grid, SDL_Renderer* renderer)
{
	SDL_Rect viewport;
	int x = 0, y = 0;

	if (!grid || !renderer)
		return 1;

	SDL_RenderGetViewport(renderer, &viewport);

	for (y = 0; y < grid->rows; y++)
	{
		for (x = 0; x < grid->cols; x++)
		{
			unsigned char r = 0, g = 0, b = 0;
			int scale = viewport.w / grid->cols;
			SDL_Rect rect;
			rect.x = x * scale;
			rect.y = y * scale;
			rect.w = scale;
			rect.h = scale;

			if (grid->cells[y * grid->cols + x])
				r = g = b = 0x7F;
			else
			{
				r = 0xEF;
				g = 0xE4;
				b = 0xB0;
			}

			if (SDL_SetRenderDrawColor(renderer, r, g, b, SDL_ALPHA_OPAQUE))
				return 1;

			if (SDL_RenderFillRect(renderer, &rect))
				return 1;
		}
	}

	SDL_RenderPresent(renderer);
	return 0;
}
