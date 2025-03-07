/*
 SuperCollider real time audio synthesis system
 Copyright (c) 2002 James McCartney. All rights reserved.
	http://www.audiosynth.com

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
 */

/*
 sc.lfnoise1~
 (c) stephen lumenta under GPL
 http://www.gnu.org/licenses/gpl.html

 part of sc-max http://github.com/sbl/sc-max
 see README
 */

#include "c74_msp.h"
#include "scmax.h"

using namespace c74::max;

static t_class *lfnoise_class = nullptr;

struct t_lfnoise {
    t_pxobject ob;

    double m_freq;
    short m_connected;
    double m_level, m_slope;
    double m_sr;
    int m_counter;

    RGen rgen;
};

void *lfnoise_new(double freq) {
    t_lfnoise *self = NULL;
    self = (t_lfnoise *)object_alloc(lfnoise_class);

    if (!self) return self;

    dsp_setup((t_pxobject *)self, 1);
    outlet_new((t_object *)self, "signal");

    self->rgen.init(sc_randomSeed());
    self->m_freq = freq <= 0 ? 500 : freq;
    self->m_counter = 0;
    self->m_level = self->rgen.frand2();
    self->m_slope = 0.f;
    self->m_sr = sys_getsr();

    return self;
}

void lfnoise_float(t_lfnoise *x, double freq) {
    x->m_freq = freq;
}

void lfnoise_perform64(t_lfnoise* self,
                       t_object* dsp64,
                       double** ins,
                       long numins,
                       double** outs,
                       long numouts,
                       long sampleframes,
                       long flags,
                       void* userparam) {
    
    double *out = outs[0];
    int remain = sampleframes;

    double freq = self->m_connected ? *ins[0] : self->m_freq;
    double level = self->m_level;
    double slope = self->m_slope;
    long counter = self->m_counter;

    if (self->ob.z_disabled) return;

    RGET
    do {
        if (counter<=0) {
            counter = (long)(self->m_sr / sc_max(freq, .001f));
            counter = sc_max(1, counter);
            float nextlevel = frand2(s1,s2,s3);
            slope = (nextlevel - level) / counter;
        }
        int nsmps = sc_min(remain, counter);
        remain -= nsmps;
        counter -= nsmps;

        while (nsmps--) {
            *out++ = level;
            level += slope;
        }

    } while (remain);

    self->m_level  = level;
    self->m_slope  = slope;
    self->m_counter= counter;
    RPUT
}

void lfnoise_dsp64(t_lfnoise *self,
                   t_object* dsp64,
                   short* count,
                   double samplerate,
                   long maxvectorsize,
                   long flags) {
    self->m_sr = samplerate;
    self->m_connected = count[0];
    object_method(dsp64, gensym("dsp_add64"), (t_object*)self, lfnoise_perform64, 0, NULL);

    // object_method_direct(void, (t_object*, t_object*, t_perfroutine64, long, void*),
    //                      dsp64, gensym("dsp_add64"), (t_object*)self, (t_perfroutine64)lfnoise_perform64, 0, NULL);
}

void lfnoise_assist(t_lfnoise *x, void *b, long m, long a, char *s) {
    if (m == ASSIST_INLET) { //inlet
        sprintf(s, "(signal/float) set freq");
    }
    else {	// outlet
        sprintf(s, "(signal) lfnoise");
    }
}

void ext_main(void* r) {
    t_class *c;
    c = class_new("sc.lfnoise1~", (method)lfnoise_new, (method)dsp_free, (long)sizeof(t_lfnoise), 0L, A_DEFFLOAT, 0);

    class_addmethod(c, (method)lfnoise_dsp64, "dsp64", A_CANT, 0);
    class_addmethod(c, (method)lfnoise_assist, "assist", A_CANT, 0);
    class_addmethod(c, (method)lfnoise_float, "float", A_FLOAT, 0);

    class_dspinit(c);
    class_register(CLASS_BOX, c);
    lfnoise_class = c;
}
