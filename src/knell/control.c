#include "control.h"

/**
 * Indicates whether the game should continue processing frames.
 */
static bool running = true;

/**
 * Should continue ticking?
 */
bool Main_IsRunning(void)
{
	return running;
}

/**
 * Tells the game to gracefully shutdown.  The program finishes the current
 * tick before shutting down.
 */
void Main_QueueGracefulShutdown(void)
{
	running = false;
}
