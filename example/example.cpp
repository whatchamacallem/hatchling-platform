// SPDX-FileCopyrightText: © 2017-2025 Adrian Johnston.
// SPDX-License-Identifier: MIT
// This file is licensed under the MIT license found in the LICENSE.md file.

// All symbols declared in this file must contain with hxexample_.

#include <hx/hatchling.h>
#include <hx/hxconsole.hpp>
#include <hx/hxfile.hpp>
#include <hx/hxmemory_manager.h>
#include <hx/hxprofiler.hpp>
#include <hx/hxthread.hpp>
#include <hx/hxtask_queue.hpp>

#include <math.h>

namespace {

const char s_hxexample_palette[] =
    " `.-':_,^=;><+!rc*/z?sLTv)J7(|Fi{C}fI31tlu[neoZ5Yxjya]2ESwqkP6h9d4VpOGbUAKXHm8RD#$Bg0MNWQ%&@";

// ----------------------------------------------------------------------------
// Example RAII mutex. Of course a C atomic would be simpler.

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

bool hxexample_render_cmd(void);
bool hxexample_profile_dump(void)    { hxprofiler_write_to_chrome_tracing("profile.json"); return true; }
bool hxexample_quit(void) {
    hxunique_lock lock_(g_hxexample_quit_mutex);
    g_hxexample_quit = true;
    return true;
}

bool hxexample_left(double amount)  { s_hxexample_center_x -= amount * s_hxexample_zoom;  return true; }
bool hxexample_right(double amount) { s_hxexample_center_x += amount * s_hxexample_zoom;  return true; }
bool hxexample_up(double amount)    { s_hxexample_center_y -= amount * s_hxexample_zoom;  return true; }
bool hxexample_down(double amount)  { s_hxexample_center_y += amount * s_hxexample_zoom;  return true; }
bool hxexample_in(double factor)    { s_hxexample_zoom /= factor;                         return true; }
bool hxexample_out(double factor)   { s_hxexample_zoom *= factor;                         return true; }

hxconsole_variable_named(s_hxexample_center_x, center_x);
hxconsole_variable_named(s_hxexample_center_y, center_y);
hxconsole_variable_named(s_hxexample_zoom, zoom);

hxconsole_command_named(hxexample_render_cmd, render);
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
    hxexample_row_task(void) : m_rows_(hxnull) { }

    void reset(int row_, double center_x_, double center_y_, double zoom_, int max_iter_, char (*rows_)[81]) {
        m_row_      = row_;
        m_center_x_ = center_x_;
        m_center_y_ = center_y_;
        m_zoom_     = zoom_;
        m_max_iter_ = max_iter_;
        m_rows_     = rows_;
    }

    void execute(hxtask_queue*) override {
        hxprofile_scope("row");
        const double col_scale = m_zoom_ / 80.0;
        // Terminal chars are ~2x taller than wide; halve the y-step for square pixels.
        const double row_scale = col_scale * 0.5;
        const double im0 = m_center_y_ + ((double)m_row_ - 39.5) * row_scale;
        char* dst_ = m_rows_[m_row_];
        for(int col = 0; col < 80; ++col) {
            const double re0 = m_center_x_ + ((double)col - 39.5) * col_scale;
            double re = 0.0;
            double im = 0.0;
            int iter = 0;
            while(iter < m_max_iter_) {
                const double re2 = re * re;
                const double im2 = im * im;
                if(re2 + im2 > 4.0) {
                    break;
                }
                im = 2.0 * re * im + im0;
                re = re2 - im2 + re0;
                ++iter;
            }
            dst_[col] = (iter == m_max_iter_) ? '@' : s_hxexample_palette[iter * 95 / m_max_iter_];
        }
        dst_[80] = '\0';
    }

    const char* get_label(void) const override { return "row"; }

private:
    int    m_row_;
    double m_center_x_;
    double m_center_y_;
    double m_zoom_;
    int    m_max_iter_;
    char (*m_rows_)[81];
};

// ----------------------------------------------------------------------------

static bool hxexample_render(hxtask_queue& queue_, hxexample_row_task* tasks_) {
    int max_iter = (int)(50.0 * ::sqrt(::sqrt(1.0 / (double)s_hxexample_zoom))) + 20;
    if(max_iter < 64)   { max_iter = 64; }
    if(max_iter > 4096) { max_iter = 4096; }

    hxsystem_allocator_scope scope_(hxsystem_allocator_temporary_stack);
    char (*rows)[81] = (char(*)[81])hxmalloc(80 * 81 * sizeof(char));

    hxprofiler_start();

    for(int row = 0; row < 80; ++row) {
        tasks_[row].reset(row, s_hxexample_center_x, s_hxexample_center_y, s_hxexample_zoom, max_iter, rows);
        queue_.enqueue(&tasks_[row]);
    }
    queue_.wait_for_all();

    hxprofiler_stop();
    hxprofiler_write_to_chrome_tracing("profile.json");

    hxout.print("center (%.6g, %.6g)  zoom %.6g\n",
        (double)s_hxexample_center_x, (double)s_hxexample_center_y, (double)s_hxexample_zoom);
    for(int row = 0; row < 80; ++row) {
        hxout.print("%s\n", rows[row]);
    }
    hxfree(rows);
    return true;
}

int main(void) {
    hxinit();

    ::signal(SIGINT, hxexample_notify_sigint);

    int exit_code = 0;
    {
        // Allocate the task queue and row task array once at startup; freed before shutdown.
        hxtask_queue queue_(89u, 9u);
        hxexample_row_task* tasks_ = (hxexample_row_task*)hxmalloc(80 * sizeof(hxexample_row_task));

        for(int i = 0; i < 80; ++i) {
            ::new(&tasks_[i]) hxexample_row_task();
        }

        if(!hxconsole_exec_filename("example.cfg")) {
            hxout.print("error: example.cfg not found or failed to execute\n");
            exit_code = 1;
        } else {
            char line[256];
            for(;;) {
                {
                    hxunique_lock lock_(g_hxexample_quit_mutex);
                    if(g_hxexample_quit) { break; }
                }
                hxout.print("> ");
                if(::fgets(line, (int)sizeof line, stdin) == hxnull) {
                    break;
                }
                hxconsole_exec_line(line);
                hxexample_render(queue_, tasks_);
            }
            queue_.wait_for_all();
        }

        for(int i = 0; i < 80; ++i) {
            tasks_[i].~hxexample_row_task();
        }
        hxfree(tasks_);
    }

    hxshutdown();
    return exit_code;
}
