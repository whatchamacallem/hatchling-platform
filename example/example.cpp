// SPDX-FileCopyrightText: © 2017-2025 Adrian Johnston.
// SPDX-License-Identifier: MIT
// This file is licensed under the MIT license found in the LICENSE.md file.
//
// An interactive Mandelbrot viewer.
//
// The Mandelbrot set is a fractal defined in the complex plane, constituting
// one of the most studied and visually recognised objects in mathematics. It is
// formally defined as the set of all complex numbers C for which the sequence
// Z(n+1) = Z(n)² + C, with Z(0) = 0, remains bounded as n tends to infinity.

#include <hx/hatchling.h>
#include <hx/hxarray.hpp>
#include <hx/hxconsole.hpp>
#include <hx/hxfile.hpp>
#include <hx/hxprofiler.hpp>
#include <hx/hxthread.hpp>
#include <hx/hxtask_queue.hpp>

#include <stdio.h>
#include <math.h>

namespace {

const char s_hxexample_palette[] =
	" `.-':,^=;><+!rc*/z?sLTv)J7(|Fi{C}fI31tlu[neoZ5Yxjya]2ESwqkP6h9d4VpOGbUAKXHm8RD#$Bg0MNWQ%&@";

// ----------------------------------------------------------------------------
// Handle exiting. Example of a hxmutex. Of course a C atomic would be simpler.

hxmutex g_hxexample_exit_mutex;
bool g_hxexample_exit = false;

static void hxexample_notify_sigint(int) {
	hxunique_lock lock_(g_hxexample_exit_mutex);
	g_hxexample_exit = true;
}

// ----------------------------------------------------------------------------
// Console variables and functions. Console utilization is intended to be local
// to each translation unit.

double s_hxexample_center_x = 0.0;
double s_hxexample_center_y = 0.0;
double s_hxexample_zoom = 3.0;

bool hxexample_exit(void) {
	hxunique_lock lock_(g_hxexample_exit_mutex);
	g_hxexample_exit = true;
	return true;
}

bool hxexample_profile_dump(void)   { hxprofiler_write_to_chrome_tracing("profile.json"); return true; }
bool hxexample_left(double amount)  { s_hxexample_center_x -= amount * s_hxexample_zoom;  return true; }
bool hxexample_right(double amount) { s_hxexample_center_x += amount * s_hxexample_zoom;  return true; }
bool hxexample_up(double amount)	{ s_hxexample_center_y -= amount * s_hxexample_zoom;  return true; }
bool hxexample_down(double amount)  { s_hxexample_center_y += amount * s_hxexample_zoom;  return true; }
bool hxexample_in(double factor)	{ s_hxexample_zoom /= factor;						 return true; }
bool hxexample_out(double factor)   { s_hxexample_zoom *= factor;						 return true; }

hxconsole_variable_named(s_hxexample_center_x, center_x);
hxconsole_variable_named(s_hxexample_center_y, center_y);
hxconsole_variable_named(s_hxexample_zoom, zoom);

hxconsole_command_named(hxexample_profile_dump, profile_dump);
hxconsole_command_named(hxexample_exit, exit);
hxconsole_command_named(hxexample_left, left);
hxconsole_command_named(hxexample_right, right);
hxconsole_command_named(hxexample_up, up);
hxconsole_command_named(hxexample_down, down);
hxconsole_command_named(hxexample_in, in);
hxconsole_command_named(hxexample_out, out);

} // namespace

// ----------------------------------------------------------------------------

class hxexample_row_task : public hxtask {
public:
	void set(size_t row, double center_x, double center_y, double zoom, size_t max_iter, char* row_buffer) {
		m_row		 = row;
		m_center_x   = center_x;
		m_center_y   = center_y;
		m_zoom	     = zoom;
		m_max_iter   = max_iter;
		m_row_buffer = row_buffer;
	}

	void execute(hxtask_queue*) override {
		hxprofile_scope("row");

		const double col_scale = m_zoom / 80.0;

		// Terminal chars are ~2x taller than wide; halve the y-step for square pixels.
		const double row_scale = col_scale * 0.5;
		const double imaginary_origin = m_center_y + ((double)m_row - 19.5) * row_scale;
		char* dst = m_row_buffer;

		for(size_t col = 0; col < 80; ++col) {
			const double real_origin = m_center_x + ((double)col - 39.5) * col_scale;
			double real = 0.0;
			double imaginary = 0.0;
			size_t iter = 0;
			while(iter < m_max_iter) {
				const double real_squared = real * real;
				const double imaginary_squared = imaginary * imaginary;
				if(real_squared + imaginary_squared > 4.0) {
					break;
				}
				imaginary = 2.0 * real * imaginary + imaginary_origin;
				real = real_squared - imaginary_squared + real_origin;
				++iter;
			}
			const size_t palette_size = sizeof s_hxexample_palette - 1u;
			const size_t palette_index = hxmin((size_t)(iter * 91 / m_max_iter), palette_size - 1u);
			dst[col] = (iter == m_max_iter) ? '@' : s_hxexample_palette[palette_index];
		}
		dst[80] = '\n';
		dst[81] = '\0';
	}

	const char* get_label(void) const override { return "row"; }
	
private:
	size_t	   m_row;
	double m_center_x;
	double m_center_y;
	double m_zoom;
	size_t	   m_max_iter;
	char*  m_row_buffer;
};

// ----------------------------------------------------------------------------

static bool hxexample_render(hxtask_queue& queue, hxarray<hxexample_row_task, 40u>& tasks,
		hxarray<hxarray<char, 82u>, 80u>& row_storage) {
	size_t max_iter = (size_t)(50.0 * ::sqrt(::sqrt(1.0 / (double)s_hxexample_zoom))) + 20;
	if(max_iter < 64)   { max_iter = 64; }
	if(max_iter > 4096) { max_iter = 4096; }

	hxprofiler_start();

	for(size_t row = 0u; row < 40u; ++row) {
		tasks[row].set(row, s_hxexample_center_x, s_hxexample_center_y, s_hxexample_zoom,
			max_iter, row_storage[row].data());
		queue.enqueue(&tasks[row]);
	}
	queue.wait_for_all();

	hxprofiler_stop();
	hxprofiler_write_to_chrome_tracing("profile.json");

	hxout.print("center (%.6g, %.6g) zoom %.6g\n",
		(double)s_hxexample_center_x, (double)s_hxexample_center_y, (double)s_hxexample_zoom);

	for(size_t row = 0; row < 80; ++row) {
		hxout.print("%s", row_storage[(size_t)row].data());
	}
	return true;
}

int main(void) {
	hxinit();

	::signal(SIGINT, hxexample_notify_sigint);

	int exit_code = EXIT_SUCCESS;
	{
		hxarray<hxarray<char, 82u>, 40u> row_storage(40u);
		for(auto& row : row_storage) {
			row.resize(82u);
		}

		hxtask_queue queue(40u, 8u);
		hxarray<hxexample_row_task, 40u> tasks(40u);

		if(!hxconsole_exec_filename("example.cfg")) {
			hxout << "error: example.cfg not found or failed to execute\n";
			exit_code = EXIT_FAILURE;
		} else {
			hxexample_render(queue, tasks, row_storage);
			hxout << "Type help to list commands or exit to exit."


			char line[256];
			for(;;) {
				{
					hxunique_lock lock_(g_hxexample_exit_mutex);
					if(g_hxexample_exit) { break; }
				}
				hxout << "> ";
				if(::fgets(line, (int)sizeof line, stdin) == hxnull) {
					break;
				}
				if(hxconsole_exec_line(line)) {
					hxexample_render(queue, tasks, row_storage);
				}
			}
			queue.wait_for_all();
		}
	}

	hxshutdown();
	return exit_code;
}
