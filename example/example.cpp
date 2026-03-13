// SPDX-FileCopyrightText: © 2017-2025 Adrian Johnston.
// SPDX-License-Identifier: MIT
// This file is licensed under the MIT license found in the LICENSE.md file.

#include <hx/hatchling.h>
#include <hx/hxconsole.hpp>
#include <hx/hxfile.hpp>
#include <hx/hxmemory_manager.h>
#include <hx/hxprofiler.hpp>
#include <hx/hxtask_queue.hpp>
#include <math.h>
#include <signal.h>
#include <stdatomic.h>
#include <stdio.h>
#include <string.h>

// ----------------------------------------------------------------------------

namespace {
    
const char s_palette[] =
    " `.-':_,^=;><+!rc*/z?sLTv)J7(|Fi{C}fI31tlu[neoZ5Yxjya]2ESwqkP6h9d4VpOGbUAKXHm8RD#$Bg0MNWQ%&@";

atomic_int g_hxquit = 0;

static void hxexample_notify_sigint(int) {
    atomic_store(&g_hxquit, 1);
}

// ----------------------------------------------------------------------------
// Console variables and functions. Utilization is intended to be local to each
// translation unit.  

double s_hxcenter_x = 0.0f;
double s_hxcenter_y = 0.0f;
double s_hxzoom = 3.0f;

bool hxexample_profile_dump(void)    { hxprofiler_write_to_chrome_tracing("profile.json"); return true; }
bool hxexample_quit(void)            { atomic_store(&g_hxquit, 1); return true; }

bool hxexample_left(double amount_)  { s_hxcenter_x -= amount_ * s_hxzoom; return true; }
bool hxexample_right(double amount_) { s_hxcenter_x += amount_ * s_hxzoom; return true; }
bool hxexample_up(double amount_)    { s_hxcenter_y -= amount_ * s_hxzoom; return true; }
bool hxexample_down(double amount_)  { s_hxcenter_y += amount_ * s_hxzoom; return true; }
bool hxexample_in(double factor_)    { s_hxzoom /= factor_; return true; }
bool hxexample_out(double factor_)   { s_hxzoom *= factor_; return true; }

hxconsole_variable_named(s_hxcenter_x, center_x);
hxconsole_variable_named(s_hxcenter_y, center_y);
hxconsole_variable_named(s_hxzoom, zoom);

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

struct hxexample_state {
    hxtask_queue& queue_;
    hxexample_row_task* tasks_;
};

// ----------------------------------------------------------------------------
// Row task

class hxexample_row_task : public hxtask {
public:
    int row_;
    const double* center_x_;
    const double* center_y_;
    const double* zoom_;
    int max_iter_;
    char (*rows_)[81];

    void execute(hxtask_queue*) override {
        hxprofile_scope("row");
        const int max_iter = max_iter_;
        const double cx = *center_x_;
        const double cy = *center_y_;
        const double zoom = *zoom_;
        const double col_scale = zoom / 80.0f;
        // Terminal chars are ~2x taller than wide; halve the y-step for square pixels.
        const double row_scale = col_scale * 0.5f;
        const double im0 = cy + ((double)row_ - 39.5f) * row_scale;
        char* dst_ = rows_[row_];
        for(int col = 0; col < 80; ++col) {
            const double re0 = cx + ((double)col - 39.5f) * col_scale;
            double re = 0.0f;
            double im = 0.0f;
            int iter = 0;
            while(iter < max_iter) {
                const double re2 = re * re;
                const double im2 = im * im;
                if(re2 + im2 > 4.0f) {
                    break;
                }
                im = 2.0f * re * im + im0;
                re = re2 - im2 + re0;
                ++iter;
            }
            dst_[col] = (iter == max_iter) ? '@' : s_palette[iter * 95 / max_iter];
        }
        dst_[80] = '\0';
    }

    const char* get_label(void) const override { return "row"; }
};

// ----------------------------------------------------------------------------
// Render

static bool hxexample_render(hxexample_state& state_) {
    int max_iter = (int)(50.0 * ::sqrt(::sqrt(1.0 / (double)s_hxzoom))) + 20;
    if(max_iter < 64)   { max_iter = 64; }
    if(max_iter > 4096) { max_iter = 4096; }

    hxsystem_allocator_scope scope_(hxsystem_allocator_temporary_stack);
    char (*rows_)[81] = (char(*)[81])hxmalloc(80 * 81 * sizeof(char));

    hxprofiler_start();

    for(int row = 0; row < 80; ++row) {
        hxexample_row_task* t_ = &state_.tasks_[row];
        t_->row_      = row;
        t_->center_x_ = &s_hxcenter_x;
        t_->center_y_ = &s_hxcenter_y;
        t_->zoom_     = &s_hxzoom;
        t_->max_iter_ = max_iter;
        t_->rows_     = rows_;
        state_.queue_.enqueue(t_);
    }
    state_.queue_.wait_for_all();

    hxprofiler_stop();
    hxprofiler_write_to_chrome_tracing("profile.json");

    hxlogconsole("center (%.6g, %.6g)  zoom %.6g\n",
        (double)s_hxcenter_x, (double)s_hxcenter_y, (double)s_hxzoom);
    for(int row = 0; row < 80; ++row) {
        hxlogconsole("%s\n", rows_[row]);
    }
    hxfree(rows_);
    return true;
}

// ----------------------------------------------------------------------------
// Entry point

int main(void) {
    hxinit();

    ::signal(SIGINT, hxexample_notify_sigint);

    int exit_code_ = 0;
    {
        // Allocate the task queue and row task array once at startup; freed at shutdown.
        hxtask_queue queue_(89u, 9u);
        hxexample_row_task* tasks_ = (hxexample_row_task*)hxmalloc(80 * sizeof(hxexample_row_task));
        for(int i = 0; i < 80; ++i) {
            ::new(&tasks_[i]) hxexample_row_task();
        }

        hxexample_state state_ = { queue_, tasks_ };
        s_hxstate = &state_;

        if(!hxconsole_exec_filename("example.cfg")) {
            hxlogconsole("error: example.cfg not found or failed to execute\n");
            exit_code_ = 1;
        } else {
            char line_[256];
            while(!atomic_load(&g_hxquit)) {
                hxlogconsole("> ");
                if(::fgets(line_, (int)sizeof line_, stdin) == hxnull) {
                    break;
                }
                hxconsole_exec_line(line_);
                hxexample_render(*s_hxstate);
            }
            queue_.wait_for_all();
        }

        s_hxstate = hxnull;
        for(int i = 0; i < 80; ++i) { tasks_[i].~hxexample_row_task(); }
        hxfree(tasks_);
    } // queue_ destructs here, before hxshutdown

    hxshutdown();
    return exit_code_;
}
