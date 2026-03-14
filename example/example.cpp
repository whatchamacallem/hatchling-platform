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
#include <hx/hxmemory_manager.h>
#include <hx/hxprofiler.hpp>
#include <hx/hxthread.hpp>
#include <hx/hxtask_queue.hpp>

#include <math.h>

namespace {

const char s_hxexample_palette[] =
    " `.-':,^=;><+!rc*/z?sLTv)J7(|Fi{C}fI31tlu[neoZ5Yxjya]2ESwqkP6h9d4VpOGbUAKXHm8RD#$Bg0MNWQ%&@";

// ----------------------------------------------------------------------------
// Handle quitting. Example of a hxmutex. Of course a C atomic would be simpler.

hxmutex g_hxexample_quit_mutex;
bool g_hxexample_quit = false;

static void hxexample_notify_sigint(int) {
    hxunique_lock lock_(g_hxexample_quit_mutex);
    g_hxexample_quit = true;
}

// ----------------------------------------------------------------------------
// Console variables and functions. Console utilization is intended to be local
// to each translation unit.

double s_hxexample_center_x = 0.0;
double s_hxexample_center_y = 0.0;
double s_hxexample_zoom = 3.0;

bool hxexample_quit(void) {
    hxunique_lock lock_(g_hxexample_quit_mutex);
    g_hxexample_quit = true;
    return true;
}

bool hxexample_profile_dump(void)   { hxprofiler_write_to_chrome_tracing("profile.json"); return true; }
bool hxexample_left(double amount)  { s_hxexample_center_x -= amount * s_hxexample_zoom;  return true; }
bool hxexample_right(double amount) { s_hxexample_center_x += amount * s_hxexample_zoom;  return true; }
bool hxexample_up(double amount)    { s_hxexample_center_y -= amount * s_hxexample_zoom;  return true; }
bool hxexample_down(double amount)  { s_hxexample_center_y += amount * s_hxexample_zoom;  return true; }
bool hxexample_in(double factor)    { s_hxexample_zoom /= factor;                         return true; }
bool hxexample_out(double factor)   { s_hxexample_zoom *= factor;                         return true; }

hxconsole_variable_named(s_hxexample_center_x, center_x);
hxconsole_variable_named(s_hxexample_center_y, center_y);
hxconsole_variable_named(s_hxexample_zoom, zoom);

hxconsole_command_named(hxexample_profile_dump, profile_dump);
hxconsole_command_named(hxexample_quit, quit);
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
    void set(int row, double center_x, double center_y, double zoom, int max_iter, char* row_buffer) {
        m_row        = row;
        m_center_x   = center_x;
        m_center_y   = center_y;
        m_zoom       = zoom;
        m_max_iter   = max_iter;
        m_row_buffer = row_buffer;
    }

    void execute(hxtask_queue*) override {
        hxprofile_scope("row");

        const double col_scale = m_zoom / 80.0;

        // Terminal chars are ~2x taller than wide; halve the y-step for square pixels.
        const double row_scale = col_scale * 0.5;
        const double imaginary_origin = m_center_y + ((double)m_row - 39.5) * row_scale;
        char* dst = m_row_buffer;

        for(int col = 0; col < 80; ++col) {
            const double real_origin = m_center_x + ((double)col - 39.5) * col_scale;
            double real = 0.0;
            double imaginary = 0.0;
            int iter = 0;
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
            dst[col] = (iter == m_max_iter) ? '@' : s_hxexample_palette[iter * 95 / m_max_iter];
        }
        dst[80] = '\n';
        dst[81] = '\0';
    }

    const char* get_label(void) const override { return "row"; }

private:
    int    m_row;
    double m_center_x;
    double m_center_y;
    double m_zoom;
    int    m_max_iter;
    char*  m_row_buffer;
};

// ----------------------------------------------------------------------------

static bool hxexample_render(hxtask_queue& queue, hxexample_row_task* tasks,
        hxarray<hxarray<char, 82>, 80>& row_storage) {
    int max_iter = (int)(50.0 * ::sqrt(::sqrt(1.0 / (double)s_hxexample_zoom))) + 20;
    if(max_iter < 64)   { max_iter = 64; }
    if(max_iter > 4096) { max_iter = 4096; }

    hxprofiler_start();

    for(int row = 0; row < 80; ++row) {
        tasks[row].set(row, s_hxexample_center_x, s_hxexample_center_y, s_hxexample_zoom,
            max_iter, row_storage[row].data());
        queue.enqueue(&tasks[row]);
    }
    queue.wait_for_all();

    hxprofiler_stop();
    hxprofiler_write_to_chrome_tracing("profile.json");

    hxout.print("center (%.6g, %.6g) zoom %.6g\n",
        (double)s_hxexample_center_x, (double)s_hxexample_center_y, (double)s_hxexample_zoom);

    for(int row = 0; row < 80; ++row) {
        hxout.print("%s", row_storage[row].data());
    }
    return true;
}

int main(void) {
    hxinit();

    ::signal(SIGINT, hxexample_notify_sigint);

    int exit_code = 0;
    {
        // Allocate the task queue, row task array and row storage once at startup; freed before shutdown.
        hxtask_queue queue(80u, 8u);
        hxarray<hxexample_row_task, 80> tasks(80u);
        hxarray<hxarray<char, 82>, 80> row_storage(80u);

        if(!hxconsole_exec_filename("example.cfg")) {
            hxout << "error: example.cfg not found or failed to execute\n";
            exit_code = 1;
        } else {
            char line[256];
            for(;;) {
                {
                    hxunique_lock lock_(g_hxexample_quit_mutex);
                    if(g_hxexample_quit) { break; }
                }
                hxout << "> ";
                if(::fgets(line, (int)sizeof line, stdin) == hxnull) {
                    break;
                }
                hxconsole_exec_line(line);
                hxexample_render(queue, tasks.data(), row_storage);
            }
            queue.wait_for_all();
        }
    }

    hxshutdown();
    return exit_code;
}
