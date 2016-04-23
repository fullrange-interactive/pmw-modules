/*
 * Copyright (C) 2002  Terence M. Welsh
 * Ported to Linux by Tugrul Galatali <tugrul@galatali.com>
 *
 * Skyrocket is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as 
 * published by the Free Software Foundation.
 *
 * Skyrocket is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <math.h>
#include <GLES/gl.h>
// #include <GL/glu.h>

#include <list>
#include <stdio.h>
#include <pthread.h>

#include "rsDefines.h"
#include "rsRand.h"
#include "rsMath.h"
#include "rsMath/rsMatrix.h"
#include "rsMath/rsQuat.h"
#include "skyrocket_smoke.h"
#include "skyrocket_flare.h"
#include "skyrocket_particle.h"
#include "skyrocket_world.h"
#include "skyrocket_shockwave.h"
#include "skyrocket_sound.h"

void playSound(unsigned int soundId);
float rsRandf2(float x,unsigned long seed31pmc,unsigned long *newSeed31pmc);
int rsRandi2(int x,unsigned long seed31pmc,unsigned long *newSeed31pmc);

#define LAUNCH1SOUND 0
#define LAUNCH2SOUND 1
#define BOOM1SOUND   2
#define BOOM2SOUND   3
#define BOOM3SOUND   4
#define BOOM4SOUND   5
#define NUKESOUND    6
#define POPPERSOUND  7
#define WHISTLESOUND 8

// skyrocket.cpp
extern std::list < particle > particles;
extern particle *addParticle (unsigned long seed);
extern int dAmbient;
extern int dSmoke;
extern int dExplosionsmoke;
extern int dWind;
extern int dFlare;
extern int dClouds;
extern int dIllumination;
extern int dSound;
extern float elapsedTime;

extern pthread_mutex_t         listMutex;

extern unsigned int flaretex[4];

// skyrocket_world.cpp
extern float clouds[CLOUDMESH + 1][CLOUDMESH + 1][9];

// skyrocket_flare.cpp
extern unsigned int flarelist[4];

// skyrocket_smoke.cpp
extern float smokeTime[SMOKETIMES];
extern int whichSmoke[WHICHSMOKES];
extern unsigned int smokelist[5];

GLfloat tex1[] = {
  0.0f, 0.0f,
  1.0f, 0.0f,
  0.0f, 1.0f,
  1.0f, 1.0f
};

GLfloat vtx1[] = {
  -0.5f, -0.5f, 0.0f,
  0.5f, -0.5f, 0.0f,
  -0.5f, 0.5f, 0.0f,
  0.5f, 0.5f, 0.0f
};

float billboardMat[16];

particle::particle()
{
  particle(42);
}

particle::particle(unsigned long specialSeed)
{
  type                  = STAR;
  seed                  = specialSeed;
  displayList 	        = flarelist[0];
  drag 		        = 0.612f;		// terminal velocity of 20 ft/s
  t 		        = 2.0f;
  tr 		        = t;
  bright 	        = 1.0f;
  life 		        = bright;
  size                  = 30.0f;
  makeSmoke             = 0;

  smokeTimeIndex        = 0;
  smokeTrailLength      = 0.0f;
  sparkTrailLength      = 0.0f;
  depth                 = 0.0f;
}

rsVec particle::randomColor()
{
	int i = 0, j = 0, k = 0;
	rsVec color;

	switch (rsRandi2 (6,seed,&seed)) {
	case 0:
		i = 0;
		j = 1, k = 2;
		break;
	case 1:
		i = 0;
		j = 2, k = 1;
		break;
	case 2:
		i = 1;
		j = 0, k = 2;
		break;
	case 3:
		i = 1;
		j = 2, k = 0;
		break;
	case 4:
		i = 2;
		j = 0, k = 1;
		break;
	case 5:
		i = 2;
		j = 1, k = 0;
	}

	color[i] = 1.0f;
	color[j] = rsRandf2(1.0f,seed,&seed);
	color[k] = rsRandf2(0.2f,seed,&seed);

	return (color);
}



void particle::initRocket(float r1,float g1,float b1,float r2,float g2, float b2,float power,float angle)
{
  type          = ROCKET;
    
  priRGB[0]     = r1;
  priRGB[1]     = g1;
  priRGB[2]     = b1;

  secRGB[0]     = r2;
  secRGB[1]     = g2;
  secRGB[2]     = b2;
  
  angle         = angle*3.14f/180.0f; // angle: between -15 and 15 */
  
  xyz[0]        = rsRandf2(200.0f,seed,&seed) - 100.0f;
  xyz[1]        = 5.0f;
  xyz[2]        = rsRandf2(200.0f,seed,&seed) - 100.0f;
  
  lastxyz[0]    = xyz[0];
  lastxyz[1]    = 4.0f;
  lastxyz[2]    = xyz[2];
  
  vel.set(60.0f*sin(angle),60.0f*cos(angle), 0.0f);
  
  rgb.set(rsRandf2(0.7f,seed,&seed) + 0.3f, rsRandf2(0.7f,seed,&seed) + 0.3f, 0.3f);
  
  size          = 1.0f;
  drag          = 0.281f;		// terminal velocity of 50 ft/s
  t             = rsRandf2(2.0f,seed,&seed) + power; // power: between 0 and 7 */
  tr            = t;
  bright        = 0.0f;
  
  thrust        = rsRandf2(100.0f,seed,&seed) + 200.0f;
  endthrust     = rsRandf2(0.1f,seed,&seed) + 0.3f;  
  
  spin          = rsRandf2(40.0f,seed,&seed) - 20.0f;
  tilt          = rsRandf2(30.0f * (float)fabs(spin),seed,&seed);

  tiltvec.set(cos(spin),0.0f,sin(spin));
  
  printf("%f,%f,%f | %f,%f,%f\n",vel[0],vel[1],vel[2],xyz[0],xyz[1],xyz[2]);
  
//   if (!rsRandi(200)) 
//   {
//     // crash the occasional rocket
//     spin        = 0.0f;
//     tilt        = rsRandf2(100.0f,seed,&seed) + 75.0f;
//     float temp  = rsRandf2(PIx2,seed,&seed);
// 
//     tiltvec.set(cos(temp), 0.0f, sin(temp));
//   }

  makeSmoke             = 1;
  smokeTrailLength      = 0.0f;
  sparkTrailLength      = 0.0f;
  explosiontype         = 0;
  
  if (rsRandi2 (2,seed,&seed))
          playSound (LAUNCH1SOUND);
  else
          playSound (LAUNCH2SOUND);
  
}

void particle::initFountain()
{
  type          = FOUNTAIN;
  displayList   = flarelist[0];
  size          = 30.0f;
  
  // position can be defined here because these are always on the ground
  xyz[0]        = rsRandf2(300.0f,seed,&seed) - 150.0f;
  xyz[1]        = 5.0f;
  xyz[2]        = rsRandf2(300.0f,seed,&seed) - 150.0f;
  
  rgb           = randomColor();
  t             = rsRandf2(5.0f,seed,&seed) + 10.0f;
  tr            = t;
  
  bright        = 0.0f;
  makeSmoke     = 0;
}

void particle::initSpinner()
{
  type          = SPINNER;
  displayList   = flarelist[0];
  drag          = 0.612f;		// terminal velocity of 20 ft/s
  rgb           = randomColor();
  spin          = rsRandf2(3.0f,seed,&seed) + 12.0f;	// radial velocity
  tilt          = rsRandf2(PIx2,seed,&seed);	// original rotation
  
  tiltvec.set(rsRandf2(2.0f,seed,&seed) - 1.0f, rsRandf2(2.0f,seed,&seed) - 1.0f, rsRandf2(2.0f,seed,&seed) - 1.0f);
  tiltvec.normalize();	// vector around which this spinner spins
  
  t             = rsRandf2(2.0f,seed,&seed) + 6.0f;
  tr            = t;
  bright        = 0.0f;
  size          = 20.0f;
  makeSmoke     = 1;
  
  sparkTrailLength = 0.0f;
  
  if (rsRandi2 (2,seed,&seed))
          playSound (LAUNCH1SOUND);
  else
          playSound (LAUNCH2SOUND);  
}

void particle::initSmoke(rsVec pos, rsVec speed)
{
  type          = SMOKE;
  
//   displayList   = smokelist[rsRandi (5)];
  xyz           = pos;
  vel           = speed;
  
  rgb[0] = rgb[1] = rgb[2] = 0.01f * float(dAmbient);

  drag          = 2.0f;
  
  // time for each smoke particle varies and must be assigned by the particle that produces the smoke
  size          = 0.1f;
  makeSmoke     = 0;
}

void particle::initStar()
{
  type          = STAR;
  displayList   = flarelist[0];
  drag          = 0.612f;		// terminal velocity of 20 ft/s
  size          = 30.0f;
  t             = rsRandf2(1.0f,seed,&seed) + 2.0f;
  tr            = t;
  
  static int someSmoke = 0;

  makeSmoke             = whichSmoke[someSmoke];
  smokeTrailLength      = 0.0f;
  someSmoke++;
  
  if (someSmoke >= WHICHSMOKES)
    someSmoke = 0;
}

void particle::initStreamer()
{
  type                  = STREAMER;
  displayList           = flarelist[0];
  
  drag                  = 0.612f;		// terminal velocity of 20 ft/s
  size                  = 30.0f;

  t                     = rsRandf2(1.0f,seed,&seed) + 3.0f;
  tr                    = t;

  sparkTrailLength      = 0.0f;
}

void particle::initMeteor()
{
  type                  = METEOR;
  displayList           = flarelist[0];
  drag                  = 0.612f;		// terminal velocity of 20 ft/s
  
  t                     = rsRandf2(1.0f,seed,&seed) + 3.0f;
  tr                    = t;

  size                  = 20.0f;
  sparkTrailLength      = 0.0f;
}

void particle::initStarPopper()
{
  type                  = POPPER;
  displayList           = flarelist[0];
  
  drag                  = 0.4f;
  t                     = rsRandf2(1.5f,seed,&seed) + 3.0f;
  tr                    = t;
  
  makeSmoke             = 1;
  explosiontype         = STAR;
  
  size                  = 0.0f;
  
  smokeTrailLength      = 0.0f;
}

void particle::initStreamerPopper()
{
  type                  = POPPER;
  displayList           = flarelist[0];
  size                  = 0.0f;
  drag                  = 0.4f;
  
  t                     = rsRandf2(1.5f,seed,&seed) + 3.0f;
  tr                    = t;
  
  makeSmoke             = 1;
  explosiontype         = STREAMER;
  
  smokeTrailLength      = 0.0f;
}

void particle::initMeteorPopper()
{
  type                  = POPPER;
  displayList           = flarelist[0];
  size                  = 0.0f;
  drag                  = 0.4f;
  
  t                     = rsRandf2(1.5f,seed,&seed) + 3.0f;
  tr                    = t;
  
  makeSmoke             = 1;
  explosiontype         = METEOR;
  
  smokeTrailLength      = 0.0f;
}

void particle::initLittlePopper()
{
  type          = POPPER;
  displayList   = flarelist[0];
  
  drag          = 0.4f;
  t             = 4.0f * (0.5f - sin (rsRandf2(PI,seed,&seed))) + 4.5f;
  tr            = t;
  
  size          = rsRandf2(3.0f,seed,&seed) + 7.0f;
  makeSmoke     = 0;
  
  explosiontype = POPPER;
}

void particle::initBee()
{
  type                  = BEE;
  displayList           = flarelist[0];
  size                  = 10.0f;
  drag                  = 0.3f;
  t                     = rsRandf2(1.0f,seed,&seed) + 2.5f;
  tr                    = t;
  makeSmoke             = 0;
  sparkTrailLength      = 0.0f;

  // these variables will be misused to describe bee acceleration vector
  thrust                = rsRandf2(PIx2,seed,&seed) + PI;
  endthrust             = rsRandf2(PIx2,seed,&seed) + PI;
  spin                  = rsRandf2(PIx2,seed,&seed) + PI;
  
  tiltvec.set(rsRandf2(PIx2,seed,&seed), rsRandf2(PIx2,seed,&seed), rsRandf2(PIx2,seed,&seed));
}

void particle::initSucker()
{
  int i;
  particle *newp;
  rsVec color;
  float temp1, temp2, ch, sh, cp, sp;

  type          = SUCKER;
  drag          = 0.612f;		// terminal velocity of 20 ft/s
  displayList   = flarelist[2];
  
  rgb.set(1.0f, 1.0f, 1.0f);
  
  size          = 300.0f;
  t             = tr = 4.0f;
  makeSmoke     = 0;

  // make explosion
  newp          = addParticle(seed++);
  
  newp->type    = EXPLOSION;
  newp->xyz     = xyz;
  newp->vel     = vel;
  
  newp->rgb.set(1.0f, 1.0f, 1.0f);
  
  newp->size    = 200.0f;
  newp->t       = newp->tr = 4.0f;

  // Make double ring to go along with sucker
  color         = randomColor();
  temp1         = rsRandf2(PI,seed,&seed);	// heading
  temp2         = rsRandf2(PI,seed,&seed);	// pitch
  
  ch            = cos(temp1);
  sh            = sin(temp1);
  cp            = cos(temp2);
  sp            = sin(temp2);
  
  for (i = 0; i < 90; i++)
  {
          newp          = addParticle(seed++);
          
          newp->initStar();
          newp->xyz     = xyz;
          
          newp->vel[0]  = rsRandf2(1.0f,seed,&seed) - 0.5f;
          newp->vel[1]  = 0.0f;
          newp->vel[2]  = rsRandf2(1.0f,seed,&seed) - 0.5f;
          
          newp->vel.normalize();
          
          // pitch
          newp->vel[1]  = sp * newp->vel[2];
          newp->vel[2]  = cp * newp->vel[2];
          
          // heading
          temp1         = newp->vel[0];
          newp->vel[0]  = ch * temp1 + sh * newp->vel[1];
          newp->vel[1]  = -sh * temp1 + ch * newp->vel[1];
          
          // multiply velocity
          newp->vel[0] *= 350.0f + rsRandf2(30.0f,seed,&seed);
          newp->vel[1] *= 350.0f + rsRandf2(30.0f,seed,&seed);
          newp->vel[2] *= 350.0f + rsRandf2(30.0f,seed,&seed);
          
          newp->vel[0] += vel[0];
          newp->vel[1] += vel[1];
          newp->vel[2] += vel[2];
          
          newp->rgb             = color;
          newp->t               = newp->tr = rsRandf2(2.0f,seed,&seed) + 2.0f;
          newp->makeSmoke       = 0;
  }
  
  color         = randomColor();
  
  temp1         = rsRandf2(PI,seed,&seed);	// heading
  temp2         = rsRandf2(PI,seed,&seed);	// pitch
  
  ch            = cos(temp1);
  sh            = sin(temp1);
  cp            = cos(temp2);
  sp            = sin(temp2);
  
  for(i = 0; i < 90; i++)
  {
          newp = addParticle(seed++);
          
          newp->initStar();
          
          newp->xyz     = xyz;
          
          newp->vel[0]  = rsRandf2(1.0f,seed,&seed) - 0.5f;
          newp->vel[1]  = 0.0f;
          newp->vel[2]  = rsRandf2(1.0f,seed,&seed) - 0.5f;
          
          newp->vel.normalize();
          
          // pitch
          newp->vel[1] = sp * newp->vel[2];
          newp->vel[2] = cp * newp->vel[2];
          
          // heading
          temp1 = newp->vel[0];
          
          newp->vel[0] = ch * temp1 + sh * newp->vel[1];
          newp->vel[1] = -sh * temp1 + ch * newp->vel[1];
          
          // multiply velocity
          newp->vel[0] *= 600.0f + rsRandf2(50.0f,seed,&seed);
          newp->vel[1] *= 600.0f + rsRandf2(50.0f,seed,&seed);
          newp->vel[2] *= 600.0f + rsRandf2(50.0f,seed,&seed);
          
          newp->vel[0] += vel[0];
          newp->vel[1] += vel[1];
          newp->vel[2] += vel[2];
          
          newp->rgb             = color;
          newp->t               = newp->tr = rsRandf2(2.0f,seed,&seed) + 2.0f;
          newp->makeSmoke       = 0;
  }
  
     playSound (BOOM4SOUND);
  

}

void particle::initShockwave()
{
  int i;
  particle *newp;
  rsVec color;

  type  = SHOCKWAVE;
  drag  = 0.612f;		// terminal velocity of 20 ft/s
  
  rgb.set(1.0f, 1.0f, 1.0f);
  
  size  = 0.0f;
  t     = tr = 5.0f;

  // make explosion
  newp = addParticle(seed++);
  
  newp->type    = EXPLOSION;
  newp->xyz     = xyz;
  newp->vel     = vel;
  
  newp->rgb.set(1.0f, 1.0f, 1.0f);
  
  newp->size    = 300.0f;
  newp->t       = newp->tr = 3.0f;
  makeSmoke     = 0;

  // Little sphere without smoke
  color         = randomColor();
  
  for (i = 0; i < 75; i++)
  {
    newp = addParticle(seed++);
 
    newp->initStar();
    newp->xyz           = xyz;
    
    newp->vel[0]        = rsRandf2(1.0f,seed,&seed) - 0.5f;
    newp->vel[1]        = rsRandf2(1.0f,seed,&seed) - 0.5f;
    newp->vel[2]        = rsRandf2(1.0f,seed,&seed) - 0.5f;
    
    newp->vel.normalize();
    
    newp->vel *= (rsRandf2(10.0f,seed,&seed) + 100.0f);
    newp->vel += vel;
    
    newp->rgb           = color;
    newp->size          = 100.0f;
    newp->t             = newp->tr = rsRandf2(2.0f,seed,&seed) + 2.0f;
    newp->makeSmoke     = 0;
  }

  // Disk of stars without smoke
  color         = randomColor();
  
  for (i = 0; i < 150; i++)
  {
    newp = addParticle(seed++);
    newp->initStar();
    newp->drag  = 0.2f;
    newp->xyz   = xyz;
    
    newp->vel[0] = rsRandf2(1.0f,seed,&seed) - 0.5f;
    newp->vel[1] = rsRandf2(0.03f,seed,&seed) - 0.005f;
    newp->vel[2] = rsRandf2(1.0f,seed,&seed) - 0.5f;
    
    newp->vel.normalize();
    
  // multiply velocity
    newp->vel   *= (rsRandf2(30.0f,seed,&seed) + 500.0f);
    newp->vel   += vel;
    newp->rgb   = color;
    newp->size  = 50.0f;
    newp->t     = newp->tr = rsRandf2(2.0f,seed,&seed) + 3.0f;
    
    newp->makeSmoke = 0;
  }
     playSound (BOOM4SOUND);
  
}

void particle::initStretcher()
{
  int i;
  particle *newp;
  rsVec color;

  type          = STRETCHER;
  drag          = 0.612f;		// terminal velocity of 20 ft/s
  displayList   = flarelist[3];
  
  rgb.set(1.0f, 1.0f, 1.0f);
  
  size          = 0.0f;
  t             = tr = 4.0f;
  makeSmoke     = 0;

  // explosion
  newp = addParticle(seed++);
  
  newp->type            = EXPLOSION;
  newp->displayList     = flarelist[0];
  newp->xyz             = xyz;
  newp->vel             = vel;
  
  newp->rgb.set(1.0f, 0.8f, 0.6f);
  
  newp->size            = 400.0f;
  newp->t = newp->tr    = 4.0f;
  newp->makeSmoke       = 0;

  // Make triple ring to go along with stretcher
  color = priRGB;
  
  for (i = 0; i < 80; i++)
  {
    newp = addParticle(seed++);
    
    newp->initStar();
    newp->xyz           = xyz;
    
    newp->vel[0]        = rsRandf2(1.0f,seed,&seed) - 0.5f;
    newp->vel[1]        = 0.0f;
    newp->vel[2]        = rsRandf2(1.0f,seed,&seed) - 0.5f;
    
    newp->vel.normalize();
    
    newp->vel[0] *= 400.0f + rsRandf2(30.0f,seed,&seed);
    newp->vel[1] += rsRandf2(70.0f,seed,&seed) - 35.0f;
    newp->vel[2] *= 400.0f + rsRandf2(30.0f,seed,&seed);
    
    newp->vel[0] += vel[0];
    newp->vel[1] += vel[1];
    newp->vel[2] += vel[2];
    
    newp->rgb           = priRGB;
    newp->t             = newp->tr = rsRandf2(2.0f,seed,&seed) + 2.0f;
    newp->makeSmoke     = 0;
  }
  
  color = secRGB;
  
  for (i = 0; i < 80; i++)
  {
    newp = addParticle(seed++);
    
    newp->initStar();
    newp->xyz = xyz;
    
    newp->vel[0]        = rsRandf2(1.0f,seed,&seed) - 0.5f;
    newp->vel[1]        = 0.0f;
    newp->vel[2]        = rsRandf2(1.0f,seed,&seed) - 0.5f;
    
    newp->vel.normalize();
    
    newp->vel[0] *= 550.0f + rsRandf2(40.0f,seed,&seed);
    newp->vel[1] += rsRandf2(70.0f,seed,&seed) - 35.0f;
    newp->vel[2] *= 550.0f + rsRandf2(40.0f,seed,&seed);
    
    newp->vel[0] += vel[0];
    newp->vel[1] += vel[1];
    newp->vel[2] += vel[2];
    
    newp->rgb           = secRGB;
    newp->t             = newp->tr = rsRandf2(2.0f,seed,&seed) + 2.0f;
    newp->makeSmoke     = 0;
  }
  
  color = priRGB;
  
  for (i = 0; i < 80; i++)
  {
    newp = addParticle(seed++);
    
    newp->initStar();
    newp->xyz = xyz;
    
    newp->vel[0] = rsRandf2(1.0f,seed,&seed) - 0.5f;
    newp->vel[1] = 0.0f;
    newp->vel[2] = rsRandf2(1.0f,seed,&seed) - 0.5f;
    
    newp->vel.normalize();
    
    newp->vel[0] *= 700.0f + rsRandf2(50.0f,seed,&seed);
    newp->vel[1] += rsRandf2(70.0f,seed,&seed) - 35.0f;
    newp->vel[2] *= 700.0f + rsRandf2(50.0f,seed,&seed);
    
    newp->vel[0] += vel[0];
    newp->vel[1] += vel[1];
    newp->vel[2] += vel[2];
    
    newp->rgb           = priRGB;
    newp->t             = newp->tr = rsRandf2(2.0f,seed,&seed) + 2.0f;
    newp->makeSmoke     = 0;
  }
  
  playSound (BOOM4SOUND);
}

void particle::initBigmama()
{
  int i;
  particle *newp;
  rsVec color;
  float temp;

  type = BIGMAMA;
  drag = 0.612f;		// terminal velocity of 20 ft/s
  displayList = flarelist[2];
  rgb.set (0.6f, 0.6f, 1.0f);
  size = 0.0f;
  t = tr = 5.0f;
  makeSmoke = 0;

  // explosion
  newp = addParticle(seed++);
  newp->type = EXPLOSION;
  newp->xyz = xyz;
  newp->vel = vel;
  newp->drag = 0.0f;
  newp->rgb.set (0.8f, 0.8f, 1.0f);
  newp->size = 200.0f;
  newp->t = newp->tr = 2.5f;
  newp->makeSmoke = 0;

  // vertical stars
  newp = addParticle(seed++);
  newp->initStar ();
  newp->xyz = xyz;
  newp->vel = vel;
  newp->drag = 0.0f;
  newp->vel[1] += 15.0f;
  newp->rgb.set (1.0f, 1.0f, 0.9f);
  newp->size = 400.0f;
  newp->t = newp->tr = 3.0f;
  newp->makeSmoke = 0;
  newp = addParticle(seed++);
  newp->initStar();
  newp->xyz = xyz;
  newp->vel = vel;
  newp->drag = 0.0f;
  newp->vel[1] -= 15.0f;
  newp->rgb.set (1.0f, 1.0f, 0.9f);
  newp->size = 400.0f;
  newp->t = newp->tr = 3.0f;
  newp->makeSmoke = 0;
  newp = addParticle(seed++);
  newp->initStar();
  newp->xyz = xyz;
  newp->vel = vel;
  newp->drag = 0.0f;
  newp->vel[1] += 45.0f;
  newp->rgb.set (1.0f, 1.0f, 0.6f);
  newp->size = 400.0f;
  newp->t = newp->tr = 3.5f;
  newp->makeSmoke = 0;
  newp = addParticle(seed++);
  newp->initStar();
  newp->xyz = xyz;
  newp->vel = vel;
  newp->drag = 0.0f;
  newp->vel[1] -= 45.0f;
  newp->rgb.set (1.0f, 1.0f, 0.6f);
  newp->size = 400.0f;
  newp->t = newp->tr = 3.5f;
  newp->makeSmoke = 0;
  newp = addParticle(seed++);
  newp->initStar();
  newp->xyz = xyz;
  newp->vel = vel;
  newp->drag = 0.0f;
  newp->vel[1] += 75.0f;
  newp->rgb.set (1.0f, 0.5f, 0.3f);
  newp->size = 400.0f;
  newp->t = newp->tr = 4.0f;
  newp->makeSmoke = 0;
  newp = addParticle(seed++);
  newp->initStar();
  newp->xyz = xyz;
  newp->vel = vel;
  newp->drag = 0.0f;
  newp->vel[1] -= 75.0f;
  newp->rgb.set (1.0f, 0.5f, 0.3f);
  newp->size = 400.0f;
  newp->t = newp->tr = 4.0f;
  newp->makeSmoke = 0;
  newp = addParticle(seed++);
  newp->initStar();
  newp->xyz = xyz;
  newp->vel = vel;
  newp->drag = 0.0f;
  newp->vel[1] += 105.0f;
  newp->rgb.set (1.0f, 0.0f, 0.0f);
  newp->size = 400.0f;
  newp->t = newp->tr = 4.5f;
  newp->makeSmoke = 0;
  newp = addParticle(seed++);
  newp->initStar();
  newp->xyz = xyz;
  newp->vel = vel;
  newp->drag = 0.0f;
  newp->vel[1] -= 105.0f;
  newp->rgb.set (1.0f, 0.0f, 0.0f);
  newp->size = 400.0f;
  newp->t = newp->tr = 4.5f;
  newp->makeSmoke = 0;

  // Sphere without smoke
  color = randomColor ();
  for (i = 0; i < 75; i++) {
  newp = addParticle(seed++);
  newp->initStar();
  newp->xyz = xyz;
  newp->vel[0] = rsRandf2(1.0f,seed,&seed) - 0.5f;
  newp->vel[1] = rsRandf2(1.0f,seed,&seed) - 0.5f;
  newp->vel[2] = rsRandf2(1.0f,seed,&seed) - 0.5f;
  newp->vel.normalize ();
  temp = 600.0f + rsRandf2(100.0f,seed,&seed);
  newp->vel[0] *= temp;
  newp->vel[1] *= temp;
  newp->vel[2] *= temp;
  newp->vel[0] += vel[0];
  newp->vel[1] += vel[1];
  newp->vel[2] += vel[2];
  newp->rgb = color;
  newp->t = newp->tr = rsRandf2(2.0f,seed,&seed) + 2.0f;
  newp->makeSmoke = 0;
  }

  // disk of big streamers
  color = randomColor ();
  for (i = 0; i < 50; i++) {
  newp = addParticle(seed++);
  newp->initStreamer();
  newp->drag = 0.3f;
  newp->xyz = xyz;
  newp->vel[0] = rsRandf2(1.0f,seed,&seed) - 0.5f;
  newp->vel[1] = 0.0f;
  newp->vel[2] = rsRandf2(1.0f,seed,&seed) - 0.5f;
  newp->vel.normalize ();
  newp->vel[0] *= 1000.0f + rsRandf2(100.0f,seed,&seed);
  newp->vel[1] += rsRandf2(100.0f,seed,&seed) - 50.0f;
  newp->vel[2] *= 1000.0f + rsRandf2(100.0f,seed,&seed);
  newp->vel[0] += vel[0];
  newp->vel[1] += vel[1];
  newp->vel[2] += vel[2];
  newp->rgb = color;
  newp->size = 100.0f;
  newp->t = newp->tr = rsRandf2(6.0f,seed,&seed) + 3.0f;
  newp->makeSmoke = 0;
  }
  
  playSound (NUKESOUND);
  
}

void particle::initExplosion()
{
  type          = EXPLOSION;
  displayList   = flarelist[0];
  drag          = 0.612f;
  t             = 0.5f;
  tr            = t;
  bright        = 1.0f;
  life          = bright;
  size          = 100.0f;
  makeSmoke     = 0;

  // Don't do massive explosions too close to the ground
//   if((explosiontype == 19 || explosiontype == 20) && (xyz[1] < 1000.0f))
//     explosiontype = 0;

  switch(explosiontype)
  {
    case 0:
//       rgb = randomColor();
      // PMW monochrome sphere, primary color
      if(!rsRandi2 (4,seed,&seed))	// big sphere
        popSphere (225, 1000.0f, priRGB);
      else		// regular sphere
        popSphere (175, rsRandf2(100.0f,seed,&seed) + 400.0f, priRGB);
    break;
    
    case 1:
//       rgb = randomColor();
      // PMW split sphere, primary color. Secondary choosen in popSplitSphere   
      if(!rsRandi2 (4,seed,&seed))	// big split sphere
        popSplitSphere(225, 1000.0f, priRGB);
      else		// regular split sphere
        popSplitSphere(175, rsRandf2(100.0f,seed,&seed) + 400.0f, priRGB);
    break;
      
    case 2:
      // PMW mixed colors sphere, choosen in popMultiColorSphere   
      rgb.set(1.0f, 1.0f, 1.0f);
      if(!rsRandi2(4,seed,&seed))	// big mixed multicolored sphere
        popMultiColorSphere(225, 1000.0f);
      else		// regular multicolored sphere
        popMultiColorSphere(175, rsRandf2(100.0f,seed,&seed) + 400.0f);
    break;
    
    case 3:		// ring
//       rgb = randomColor();
      // PMW monochrome ring, primary color
      popRing(70, rsRandf2(100.0f,seed,&seed) + 400.0f, priRGB);
    break;
    
    case 4:		// double sphere
//       rgb = randomColor();
      // PMW double sphere, primary color and secondary assignation
      
      popSphere(90, rsRandf2(50.0f,seed,&seed) + 200.0f, secRGB);
      popSphere(150, rsRandf2(100.0f,seed,&seed) + 500.0f, priRGB);
    break;
    
    case 5:		// sphere and ring
      // PMW primary to sphere, then ring
      
//       rgb = randomColor();
      popRing(70, rsRandf2(100.0f,seed,&seed) + 500.0f, secRGB);
      popSphere(150, rsRandf2(50.0f,seed,&seed) + 200.0f, priRGB);
    break;
    
    case 6:		// Sphere of streamers
//       rgb = randomColor();
      
      // PMW primary to streamers color
      popStreamers(40, rsRandf2(100.0f,seed,&seed) + 400.0f, priRGB);
    break;
    
    case 7:		// Sphere of meteors
//       rgb = randomColor();
      
      // PMW primary to meteor color      
      popMeteors(40, rsRandf2(100.0f,seed,&seed) + 400.0f, priRGB);
    break;
    
    case 8:		// Small sphere of stars and large sphere of streamers
      // PMW primary to sphere, then meteors
      
//       rgb = randomColor();
      popStreamers(25, rsRandf2(100.0f,seed,&seed) + 500.0f, secRGB);
      popSphere(90, rsRandf2(50.0f,seed,&seed) + 200.0f, priRGB);
    break;
    
    case 9:		// Small sphere of stars and large sphere of meteors
//       rgb = randomColor();
      // PMW primary to sphere, then streamers
      
      popMeteors(25, rsRandf2(100.0f,seed,&seed) + 500.0f, secRGB);
      popSphere(90, rsRandf2(50.0f,seed,&seed) + 200.0f, priRGB);
    break;
    
    case 10:		// Sphere of streamers inside sphere of stars
      // PMW primary to sphere, then streamers
      
//       rgb = randomColor();
      popStreamers(20, rsRandf2(100.0f,seed,&seed) + 450.0f, secRGB);
      popSphere(150, rsRandf2(50.0f,seed,&seed) + 500.0f, priRGB);
    break;
    
    case 11:		// Sphere of meteors inside sphere of stars
//       rgb = randomColor();
      // PMW primary to sphere, then meteors
      
      popMeteors(20, rsRandf2(100.0f,seed,&seed) + 450.0f, secRGB);
      popSphere(150, rsRandf2(50.0f,seed,&seed) + 500.0f, priRGB);
    break;
    
    case 12:		// a few bombs that fall and explode into stars
//       rgb = randomColor();
      popStarPoppers(8, rsRandf2(100.0f,seed,&seed) + 300.0f, priRGB);
    break;
    
    case 13:		// a few bombs that fall and explode into streamers
//       rgb = randomColor();
      popStreamerPoppers(8, rsRandf2(100.0f,seed,&seed) + 300.0f, priRGB);
    break;
    
    case 14:		// a few bombs that fall and explode into meteors
//       rgb = randomColor();
      popMeteorPoppers(8, rsRandf2(100.0f,seed,&seed) + 300.0f, priRGB);
    break;
    
    case 15:		// lots of little falling firecrackers
      popLittlePoppers(250, rsRandf2(50.0f,seed,&seed) + 150.0f);
    break;
    
    case 16: // PMW BEES, primary color used
//       rgb = randomColor();
      popBees(30, 10.0f, priRGB);
    break;
    
    case 17:		// Boom!  (loud noise and flash of light)
      rgb.set(1.0f, 1.0f, 1.0f);
      size = 150.0f;
    break;
    
    // 18 is a spinner, which doesn't require explosion
    case 19:
      rgb.set(1.0f, 1.0f, 1.0f);
      initSucker();
    break;
    
    case 20:
      rgb.set(1.0f, 1.0f, 1.0f);
      initStretcher();
    break;
    
    case 100:		// these three are little explosions for poppers
      popSphere(30, 100.0f, priRGB);
    break;
    
    case 101:
      popStreamers(10, 100.0f, priRGB);
    break;
    
    case 102:
      popMeteors(10, 100.0f, priRGB);
    break;
    default:
    break;
  }
  
  
  if (explosiontype == 17)        // extra resounding boom
          playSound (BOOM4SOUND);
  // make bees and big booms whistle sometimes
  if (explosiontype == 16 || explosiontype == 17)
          if (rsRandi2 (2,seed,&seed))
                  playSound (WHISTLESOUND);
  // regular booms
  if (explosiontype <= 16 || explosiontype >= 100)
          playSound (BOOM1SOUND + rsRandi2 (3,seed,&seed));
  // sucker and stretcher take care of their own sounds

  
  


  
  
  
  
  
  
  
}














void particle::popSphere(int numParts, float v0, rsVec color)
{
  particle *newp = NULL;
  float temp;

  for (int i = 0; i < numParts; i++)
  {
    newp = addParticle(seed++);
    newp->initStar();
    newp->xyz = xyz;
    newp->vel[0] = rsRandf2(1.0f,seed,&seed) - 0.5f;
    newp->vel[1] = rsRandf2(1.0f,seed,&seed) - 0.5f;
    newp->vel[2] = rsRandf2(1.0f,seed,&seed) - 0.5f;
    newp->vel.normalize ();
    temp = v0 + rsRandf2(50.0f,seed,&seed);
    newp->vel *= temp;
    newp->vel += vel;
    newp->rgb = color;
  }

  if (!rsRandi2 (100,seed,&seed))
    newp->t = newp->tr = rsRandf2(20.0f,seed,&seed) + 5.0f;
}

void particle::popSplitSphere(int numParts, float v0, rsVec color1)
{
  particle *newp = NULL;
  rsVec color2;
  rsVec planeNormal;
  float temp;

  // PMW Split sphere secondary color;
//   color2 = randomColor();
  color2 = secRGB;
  
  planeNormal[0] = rsRandf2(1.0f,seed,&seed) - 0.5f;
  planeNormal[1] = rsRandf2(1.0f,seed,&seed) - 0.5f;
  planeNormal[2] = rsRandf2(1.0f,seed,&seed) - 0.5f;
  planeNormal.normalize();
  
  for(int i = 0; i < numParts; i++) 
  {
    newp = addParticle(seed++);
    newp->initStar();
    newp->xyz = xyz;
    newp->vel[0] = rsRandf2(1.0f,seed,&seed) - 0.5f;
    newp->vel[1] = rsRandf2(1.0f,seed,&seed) - 0.5f;
    newp->vel[2] = rsRandf2(1.0f,seed,&seed) - 0.5f;
    
    newp->vel.normalize();
    
    if(planeNormal.dot (newp->vel) > 0.0f)
      newp->rgb = color1;
    else
      newp->rgb = color2;
    
    temp = v0 + rsRandf2(50.0f,seed,&seed);
    newp->vel *= temp;
    newp->vel += vel;
  }

  if(!rsRandi2 (100,seed,&seed))
    newp->t = newp->tr = rsRandf2(20.0f,seed,&seed) + 5.0f;
}

void particle::popMultiColorSphere(int numParts, float v0)
{
  int j;
  particle *newp = NULL;
  float temp;
  rsVec color[2];

//   color[0] = randomColor ();
//   color[1] = randomColor ();
//   color[2] = randomColor ();
  // PMW multi colored color assignation
  color[0] = priRGB;
  color[1] = secRGB;  
  
  j = 0;
  
  for(int i = 0; i < numParts; i++)
  {
    newp = addParticle(seed++);
    newp->initStar();
    newp->xyz = xyz;
    newp->vel[0] = rsRandf2(1.0f,seed,&seed) - 0.5f;
    newp->vel[1] = rsRandf2(1.0f,seed,&seed) - 0.5f;
    newp->vel[2] = rsRandf2(1.0f,seed,&seed) - 0.5f;
    newp->vel.normalize ();
    temp = v0 + rsRandf2(30.0f,seed,&seed);
    newp->vel *= temp;
    newp->vel += vel;
    newp->rgb = color[j];
    j++;
    
    if(j >= 2)
      j = 0;
  }

  if(!rsRandi2 (100,seed,&seed))
    newp->t = newp->tr = rsRandf2(20.0f,seed,&seed) + 5.0f;
}

void particle::popRing(int numParts, float v0, rsVec color)
{
  particle *newp = NULL;
  float temp1, temp2;
  float ch, sh, cp, sp;

  temp1 = rsRandf2(PI,seed,&seed);	// heading
  temp2 = rsRandf2(PI,seed,&seed);	// pitch
  
  ch = cos(temp1);
  sh = sin(temp1);
  cp = cos(temp2);
  sp = sin(temp2);
  
  for (int i = 0; i < numParts; i++)
  {
    newp = addParticle(seed++);
    newp->initStar();
    
    newp->xyz = xyz;
    
    newp->vel[0] = rsRandf2(1.0f,seed,&seed) - 0.5f;
    newp->vel[1] = 0.0f;
    newp->vel[2] = rsRandf2(1.0f,seed,&seed) - 0.5f;
    newp->vel.normalize();
    
    // pitch
    newp->vel[1] = sp * newp->vel[2];
    newp->vel[2] = cp * newp->vel[2];
    
    // heading
    temp1 = newp->vel[0];
    newp->vel[0] = ch * temp1 + sh * newp->vel[1];
    newp->vel[1] = -sh * temp1 + ch * newp->vel[1];
    
    // multiply velocity
    newp->vel[0] *= v0 + rsRandf2(50.0f,seed,&seed);
    newp->vel[1] *= v0 + rsRandf2(50.0f,seed,&seed);
    newp->vel[2] *= v0 + rsRandf2(50.0f,seed,&seed);
    newp->vel += vel;
    
    newp->rgb = color;
  }

  if (!rsRandi2 (100,seed,&seed))
  newp->t = newp->tr = rsRandf2(20.0f,seed,&seed) + 5.0f;
}

void particle::popStreamers(int numParts, float v0, rsVec color)
{
  particle *newp = NULL;
  float temp;

  for (int i = 0; i < numParts; i++)
  {
    newp = addParticle(seed++);
    newp->initStreamer();
    newp->xyz = xyz;
    
    newp->vel[0] = rsRandf2(1.0f,seed,&seed) - 0.5f;
    newp->vel[1] = rsRandf2(1.0f,seed,&seed) - 0.5f;
    newp->vel[2] = rsRandf2(1.0f,seed,&seed) - 0.5f;
    newp->vel.normalize();
    
    temp = v0 + rsRandf2(50.0f,seed,&seed);
    newp->vel *= temp;
    newp->vel += vel;
    newp->rgb = color;
  }
}

void particle::popMeteors(int numParts, float v0, rsVec color)
{
  particle *newp = NULL;
  float temp;

  for (int i = 0; i < numParts; i++)
  {
    newp = addParticle(seed++);
    newp->initMeteor();
    newp->xyz = xyz;
    
    newp->vel[0] = rsRandf2(1.0f,seed,&seed) - 0.5f;
    newp->vel[1] = rsRandf2(1.0f,seed,&seed) - 0.5f;
    newp->vel[2] = rsRandf2(1.0f,seed,&seed) - 0.5f;
    newp->vel.normalize ();
    
    temp = v0 + rsRandf2(50.0f,seed,&seed);
    newp->vel *= temp;
    newp->vel += vel;
    newp->rgb = color;
  }
}

void particle::popStarPoppers(int numParts, float v0, rsVec color)
{
  particle *newp = NULL;
  float v0x2 = v0 * 2;

  for (int i = 0; i < numParts; i++)
  {
    newp = addParticle(seed++);
    newp->initStarPopper();
    newp->xyz = xyz;
    newp->vel[0] = vel[0] + rsRandf2(v0x2,seed,&seed) - v0;
    newp->vel[1] = vel[1] + rsRandf2(v0x2,seed,&seed) - v0;
    newp->vel[2] = vel[2] + rsRandf2(v0x2,seed,&seed) - v0;
    newp->rgb = color;
  }
}

void particle::popStreamerPoppers(int numParts, float v0, rsVec color)
{
  particle *newp = NULL;
  float v0x2 = v0 * 2;

  for (int i = 0; i < numParts; i++)
  {
    newp = addParticle(seed++);
    newp->initStreamerPopper();
    newp->xyz = xyz;
    
    newp->vel[0] = vel[0] + rsRandf2(v0x2,seed,&seed) - v0;
    newp->vel[1] = vel[1] + rsRandf2(v0x2,seed,&seed) - v0;
    newp->vel[2] = vel[2] + rsRandf2(v0x2,seed,&seed) - v0;
    newp->rgb = color;
  }
}

void particle::popMeteorPoppers(int numParts, float v0, rsVec color)
{
  int i;
  particle *newp;
  float v0x2 = v0 * 2;

  for (i = 0; i < numParts; i++)
  {
    newp = addParticle(seed++);
    newp->initMeteorPopper();
    newp->xyz = xyz;
    
    newp->vel[0] = vel[0] + rsRandf2(v0x2,seed,&seed) - v0;
    newp->vel[1] = vel[1] + rsRandf2(v0x2,seed,&seed) - v0;
    newp->vel[2] = vel[2] + rsRandf2(v0x2,seed,&seed) - v0;
    
    newp->rgb = color;
  }
}

void particle::popLittlePoppers(int numParts, float v0)
{
  particle *newp = NULL;
  float v0x2 = v0 * 2;

  for (int i = 0; i < numParts; i++) {
  newp = addParticle(seed++);
  newp->initLittlePopper();
  newp->xyz = xyz;
  newp->vel[0] = vel[0] + rsRandf2(v0x2,seed,&seed) - v0;
  newp->vel[1] = vel[1] + rsRandf2(v0x2,seed,&seed) - v0;
  newp->vel[2] = vel[2] + rsRandf2(v0x2,seed,&seed) - v0;
  }
  
  playSound(POPPERSOUND);
  
}

void particle::popBees(int numParts, float v0, rsVec color)
{
  particle *newp = NULL;

  for (int i = 0; i < numParts; i++)
  {
    newp = addParticle(seed++);
    newp->initBee();
    newp->xyz = xyz;
    
    newp->vel[0] = rsRandf2(1.0f,seed,&seed) - 0.5f;
    newp->vel[1] = rsRandf2(1.0f,seed,&seed) - 0.5f;
    newp->vel[2] = rsRandf2(1.0f,seed,&seed) - 0.5f;
    newp->vel *= v0;
    newp->vel += vel;
    
    newp->rgb = color;
  }
}

// Rockets and explosions illuminate smoke
// Only explosions illuminate clouds
void illuminate (particle * ill)
{
	float temp;
	// desaturate illumination colors
	float newrgb[3] = { ill->rgb[0] * 0.6f + 0.4f, ill->rgb[1] * 0.6f + 0.4f, ill->rgb[2] * 0.6f + 0.4f };

	// Smoke illumination
	if ((ill->type == ROCKET) || (ill->type == FOUNTAIN)) {
		float distsquared;

//                 pthread_mutex_lock(&listMutex);    

		std::list < particle >::iterator smk = particles.begin ();
		while (smk != particles.end ()) {
			if (smk->type == SMOKE) {
				distsquared = (ill->xyz[0] - smk->xyz[0]) * (ill->xyz[0] - smk->xyz[0])
					+ (ill->xyz[1] - smk->xyz[1]) * (ill->xyz[1] - smk->xyz[1])
					+ (ill->xyz[2] - smk->xyz[2]) * (ill->xyz[2] - smk->xyz[2]);
				if (distsquared < 40000.0f) {
					temp = (40000.0f - distsquared) * 0.000025f;
					temp = temp * temp * ill->bright;
					smk->rgb[0] += temp * newrgb[0];
					if (smk->rgb[0] > 1.0f)
						smk->rgb[0] = 1.0f;
					smk->rgb[1] += temp * newrgb[1];
					if (smk->rgb[1] > 1.0f)
						smk->rgb[1] = 1.0f;
					smk->rgb[2] += temp * newrgb[2];
					if (smk->rgb[2] > 1.0f)
						smk->rgb[2] = 1.0f;
				}
			}
			smk++;
		}
		
//                 pthread_mutex_unlock(&listMutex);    

		
	} else if (ill->type == EXPLOSION) {
		float distsquared;

//                 pthread_mutex_lock(&listMutex);    
                
		std::list < particle >::iterator smk = particles.begin ();
		while (smk != particles.end ()) {
			if (smk->type == SMOKE) {
				distsquared = (ill->xyz[0] - smk->xyz[0]) * (ill->xyz[0] - smk->xyz[0])
					+ (ill->xyz[1] - smk->xyz[1]) * (ill->xyz[1] - smk->xyz[1])
					+ (ill->xyz[2] - smk->xyz[2]) * (ill->xyz[2] - smk->xyz[2]);
				if (distsquared < 640000.0f) {
					temp = (640000.0f - distsquared) * 0.0000015625f;
					temp = temp * temp * ill->bright;
					smk->rgb[0] += temp * newrgb[0];
					if (smk->rgb[0] > 1.0f)
						smk->rgb[0] = 1.0f;
					smk->rgb[1] += temp * newrgb[1];
					if (smk->rgb[1] > 1.0f)
						smk->rgb[1] = 1.0f;
					smk->rgb[2] += temp * newrgb[2];
					if (smk->rgb[2] > 1.0f)
						smk->rgb[2] = 1.0f;
				}
			}
			smk++;
		}
		
//                  pthread_mutex_unlock(&listMutex);    


		// cloud illumination
		if (dClouds) {
			int north, south, west, east;	// limits of cloud indices to inspect
			int halfmesh = CLOUDMESH / 2;
			float distsquared;

			// remember clouds have 20000-foot radius from world.h, hence 0.00005
			// Hardcoded values like this are evil, but oh well
			south = int ((ill->xyz[2] - 1600.0f) * 0.00005f * float (halfmesh)) + halfmesh;
			north = int ((ill->xyz[2] + 1600.0f) * 0.00005f * float (halfmesh) + 0.5f) + halfmesh;
			west = int ((ill->xyz[0] - 1600.0f) * 0.00005f * float (halfmesh)) + halfmesh;
			east = int ((ill->xyz[0] + 1600.0f) * 0.00005f * float (halfmesh) + 0.5f) + halfmesh;

			for (int i = west; i <= east; i++) {
				for (int j = south; j <= north; j++) {
					distsquared = (clouds[i][j][0] - ill->xyz[0]) * (clouds[i][j][0] - ill->xyz[0])
						+ (clouds[i][j][1] - ill->xyz[1]) * (clouds[i][j][1] - ill->xyz[1])
						+ (clouds[i][j][2] - ill->xyz[2]) * (clouds[i][j][2] - ill->xyz[2]);
					if (distsquared < 2560000.0f) {
						temp = (2560000.0f - distsquared) * 0.000000390625f;
						temp = temp * temp * ill->bright;
						clouds[i][j][6] += temp * newrgb[0];
						if (clouds[i][j][6] > 1.0f)
							clouds[i][j][6] = 1.0f;
						clouds[i][j][7] += temp * newrgb[1];
						if (clouds[i][j][7] > 1.0f)
							clouds[i][j][7] = 1.0f;
						clouds[i][j][8] += temp * newrgb[2];
						if (clouds[i][j][8] > 1.0f)
							clouds[i][j][8] = 1.0f;
					}
				}
			}
		}
	}
}

// pulling of other particles
void pulling (particle * suck)
{
	rsVec diff;
	float pulldistsquared;
	float pullconst = (1.0f - suck->life) * 0.01f * elapsedTime;

//         pthread_mutex_lock(&listMutex);    
        
	std::list < particle >::iterator puller = particles.begin ();
	while (puller != particles.end ()) {
		diff = suck->xyz - puller->xyz;
		pulldistsquared = diff[0] * diff[0] + diff[1] * diff[1] + diff[2] * diff[2];
		if (pulldistsquared < 250000.0f && pulldistsquared != 0.0f && puller->type != SUCKER) {
			diff.normalize ();
			puller->vel += diff * ((250000.0f - pulldistsquared) * pullconst);
		}

		puller++;
	}
	
//        pthread_mutex_unlock(&listMutex);    
	
}

// pushing of other particles
void pushing (particle * shock)
{
	rsVec diff;
	float pushdistsquared;
	float pushconst = (1.0f - shock->life) * 0.002f * elapsedTime;

//             pthread_mutex_lock(&listMutex);            
        
	std::list < particle >::iterator pusher = particles.begin ();
	while (pusher != particles.end ()) {
		diff = pusher->xyz - shock->xyz;
		pushdistsquared = diff[0] * diff[0] + diff[1] * diff[1] + diff[2] * diff[2];
		if (pushdistsquared < 640000.0f && pushdistsquared != 0.0f && pusher->type != SHOCKWAVE) {
			diff.normalize ();
			pusher->vel += diff * ((640000.0f - pushdistsquared) * pushconst);
		}

		pusher++;
	}
	
//             pthread_mutex_unlock(&listMutex);    
	
}

// vertical stretching of other particles (x, z sucking; y pushing)
void stretching (particle * stretch)
{
	rsVec diff;
	float stretchdistsquared, temp;
	float stretchconst = (1.0f - stretch->life) * 0.002f * elapsedTime;

//             pthread_mutex_lock(&listMutex);            
        
	std::list < particle >::iterator stretcher = particles.begin ();
	while (stretcher != particles.end ()) {
		diff = stretch->xyz - stretcher->xyz;
		stretchdistsquared = diff[0] * diff[0] + diff[1] * diff[1] + diff[2] * diff[2];
		if (stretchdistsquared < 640000.0f && stretchdistsquared != 0.0f && stretcher->type != STRETCHER) {
			diff.normalize ();
			temp = (640000.0f - stretchdistsquared) * stretchconst;
			stretcher->vel[0] += diff[0] * temp * 5.0f;
			stretcher->vel[1] -= diff[1] * temp;
			stretcher->vel[2] += diff[2] * temp * 5.0f;
		}

		stretcher++;
	}
	
//             pthread_mutex_unlock(&listMutex);    
	
}

//******************************************
//  Update particles
//******************************************
void particle::update()
{
	int i;
	float temp;
	static rsVec dir, crossvec, velvec, rocketEjection;
	static rsQuat spinquat;
	static rsMatrix spinmat;
	particle *newp;

	// update velocities
	if (type == ROCKET && life > endthrust)
        {
		dir = vel;
		dir.normalize ();
		crossvec.cross (dir, tiltvec);	// correct sidevec
		tiltvec.cross (crossvec, dir);
		tiltvec.normalize ();
		spinquat.make (spin * elapsedTime, dir[0], dir[1], dir[2]);	// twist tiltvec
		spinmat.fromQuat (spinquat);
		tiltvec.transVec (spinmat);
		vel += dir * (thrust * elapsedTime);	// apply thrust
		vel += tiltvec * (tilt * elapsedTime);	// apply tilt
                
//                 printf("[update] %f,%f,%f | %f,%f,%f\n",vel[0],vel[1],vel[2],xyz[0],xyz[1],xyz[2]);
                
	}

	if (type == BEE) {
		vel[0] += 800.0f * cos (tiltvec[0]) * elapsedTime;
		vel[1] += 800.0f * (cos (tiltvec[1]) - 0.2f) * elapsedTime;
		vel[2] += 800.0f * cos (tiltvec[2]) * elapsedTime;
	} 

	if (type != SMOKE)
		vel[1] -= elapsedTime * 32.0f;	// gravity

	// apply air resistance
	temp = 1.0f / (1.0f + drag * elapsedTime);
	//temp = temp * temp;
	vel *= temp * temp;

	// update position
	// (Fountains don't move)
	if (type != FOUNTAIN) {
		lastxyz = xyz;
		xyz += vel * elapsedTime;
		// Wind:  1/10 wind on ground; -1/2 wind at 500 feet; full wind at 2000 feet;
		// This value is calculated to coincide with movement of the clouds in world.h
		// Here's the polynomial wind equation that simulates windshear:
		xyz[0] += (0.1f - 0.00175f * xyz[1] + 0.0000011f * xyz[1] * xyz[1]) * dWind * elapsedTime;
	}

	// brightness and life
	tr -= elapsedTime;
	switch (type) {
	case ROCKET:
		life = tr / t;
		if (life > endthrust) {	// Light up rocket gradually after it is launched
			bright += 2.0f * elapsedTime;
			if (bright > 1.0f)
				bright = 1.0f;
		} else {	// Darken rocket after it stops thrusting
			bright -= elapsedTime;
			if (bright < 0.0f)
				bright = 0.0f;
		}
		break;
	case SMOKE:
		life = tr / t;
		bright = life * 0.7f;
		size += (30.0f - size) * (1.2f * elapsedTime);
		break;
	case FOUNTAIN:
	case SPINNER:
		life = tr / t;
		bright = life * life;
		// dim newborn fountains and spinners
		temp = t - tr;
		if (temp < 0.5f)
			bright *= temp * 2.0f;
		break;
	case EXPLOSION:
		life = tr / t;
		bright = life * life;
		break;
	case STAR:
	case STREAMER:
	case METEOR:
		temp = (t - tr) / t;
		temp = temp * temp;
		bright = 1.0f - (temp * temp);
		life = bright;
		break;
	case POPPER:
		life = tr;
		break;
	case BEE:
		temp = (t - tr) / t;
		temp = temp * temp;
		bright = 1.0f - (temp * temp);
		life = bright;
		// Update bee acceleration (tiltvec) using misused variables
		tiltvec[0] += thrust * elapsedTime;
		tiltvec[1] += endthrust * elapsedTime;
		tiltvec[2] += spin * elapsedTime;
		break;
	case SUCKER:
		life = tr / t;
		bright = life;
		size = 250.0f * life;
		break;
	case SHOCKWAVE:
		life = tr / t;
		bright = life;
		rgb[2] = life * 0.5f + 0.5f;	// get a little yellow
		size += 400.0f * elapsedTime;
		break;
	case STRETCHER:
		life = tr / t;
		bright = 1.0f - ((1.0f - life) * (1.0f - life));
		size = 400.0f * bright;
		break;
	case BIGMAMA:
		life = tr / t;
		bright = life * 2.0f - 1.0f;
		if (bright < 0.0f)
			bright = 0.0f;
		size += 1500.0f * elapsedTime;
	}

// 	if (makeSmoke && dSmoke)
//         {
// 		rsVec diff = xyz - lastxyz;
// 
// 		// distance rocket traveled since last frame
// 		temp = diff.length ();
// 		smokeTrailLength += temp;
// 		// number of smoke puffs to release (1 every 2 feet)
// 		int puffs = int (smokeTrailLength * 0.5f);
// 		float multiplier = 2.0f / smokeTrailLength;
// 		smokeTrailLength -= float (puffs) * 2.0f;
// 		rsVec smkpos = lastxyz;
// 
// 		if ((type == ROCKET) && (life > endthrust)) {	// eject the smoke forcefully
// 			rocketEjection = vel;
// 			rocketEjection.normalize ();
// 			rocketEjection *= -2.0f * thrust * (life - endthrust);
// 			for (i = 0; i < puffs; i++) {	// make puffs of smoke
// 				smkpos += diff * multiplier;
// 				newp = addParticle(seed++);
// 				velvec[0] = rocketEjection[0] + rsRandf2(20.0f,seed,&seed) - 10.0f;
// 				velvec[1] = rocketEjection[1] + rsRandf2(20.0f,seed,&seed) - 10.0f;
// 				velvec[2] = rocketEjection[2] + rsRandf2(20.0f,seed,&seed) - 10.0f;
// 				newp->initSmoke (smkpos, velvec);
// 				newp->t = newp->tr = smokeTime[smokeTimeIndex];
// 				smokeTimeIndex++;
// 				if (smokeTimeIndex >= SMOKETIMES)
// 					smokeTimeIndex = 0;
// 			}
// 		} else {	// just form smoke in place
// 			for (i = 0; i < puffs; i++) {
// 				smkpos += diff * multiplier;
// 				newp = addParticle(seed++);
// 				velvec[0] = rsRandf2(20.0f,seed,&seed) - 10.0f;
// 				velvec[1] = rsRandf2(20.0f,seed,&seed) - 10.0f;
// 				velvec[2] = rsRandf2(20.0f,seed,&seed) - 10.0f;
// 				newp->initSmoke (smkpos, velvec);
// 				newp->t = newp->tr = smokeTime[smokeTimeIndex];
// 				smokeTimeIndex++;
// 				if (smokeTimeIndex >= SMOKETIMES)
// 					smokeTimeIndex = 0;
// 			}
// 		}
// 	}

	switch (type) {
	case ROCKET:
		// Sparks thrusting from rockets
		if (life > endthrust) {
			rsVec diff = xyz - lastxyz;

			// distance rocket traveled since last frame
			temp = diff.length ();
			sparkTrailLength += temp;
			// number of sparks to release
			int sparks = int (sparkTrailLength * 0.4f);
			sparkTrailLength -= float (sparks) * 2.5f;

			rocketEjection = vel;
			rocketEjection.normalize ();
			rocketEjection *= -thrust * (life - endthrust);
			for (i = 0; i < sparks; i++) {	// make sparks
				newp = addParticle(seed++);
				newp->initStar();
				newp->xyz = xyz - (diff * rsRandf(1.0f));
				newp->vel[0] = rocketEjection[0] + rsRandf(60.0f) - 30.0f;
				newp->vel[1] = rocketEjection[1] + rsRandf(60.0f) - 30.0f;
				newp->vel[2] = rocketEjection[2] + rsRandf(60.0f) - 30.0f;
				newp->rgb = rgb;
				newp->t = rsRandf(0.2f) + 0.1f;
				newp->tr = newp->t;
				newp->size = 8.0f * life;
				newp->displayList = flarelist[3];
				newp->makeSmoke = 0;
			}
		}
		break;

	case FOUNTAIN: {
// 		// Stars shooting up from fountain
// 		// spew 10-20 particles per second at maximum brightness
// 		sparkTrailLength += elapsedTime * bright * (rsRandf2(10.0f,seed,&seed) + 10.0f);
// 		int stars = int (sparkTrailLength);
// 		sparkTrailLength -= float(stars);
// 
// 		for (i = 0; i < stars; i++) {
// 			newp = addParticle(seed++);
// 			newp->initStar();
// 			newp->drag = 0.342f;	// terminal velocity is 40 ft/s
// 			newp->xyz = xyz;
// 			newp->xyz[1] += rsRandf2(elapsedTime * 100.0f,seed,&seed);
// 			if (newp->xyz[1] > 50.0f)
// 				newp->xyz[1] = 50.0f;
// 			newp->vel.set (rsRandf2(20.0f,seed,&seed) - 10.0f, rsRandf2(30.0f,seed,&seed) + 100.0f, rsRandf2(20.0f,seed,&seed) - 10.0f);
// 			newp->size = 10.0f;
// 			newp->rgb = rgb;
// 			newp->makeSmoke = 0;
// 		}

		}
		break;

	case SPINNER: {
		// Stars shooting out from spinner
		dir.set (1.0f, 0.0f, 0.0f);
		crossvec.cross (dir, tiltvec);
		crossvec.normalize ();
		crossvec *= 400.0f;
		temp = spin * elapsedTime;	// radius of spin this frame
		// spew 90-100 particles per second at maximum brightness
// 		sparkTrailLength += elapsedTime * bright * (rsRandf2(10.0f,seed,&seed) + 90.0f);

                sparkTrailLength += elapsedTime * bright * 90.0f;


		int stars = int (sparkTrailLength);
		sparkTrailLength -= float (stars);

		for (i = 0; i < stars; i++) {
			spinquat.make (tilt /*+ rsRandf2(temp,seed,&seed)*/, tiltvec[0], tiltvec[1], tiltvec[2]);
			spinquat.toMat (spinmat.m);
			newp = addParticle(seed++);
			newp->initStar();
			newp->xyz = xyz;
			newp->vel.set (vel[0] - (spinmat[0] * crossvec[0] + spinmat[4] * crossvec[1] + spinmat[8] * crossvec[2]) +/* rsRandf2(20.0f,seed,&seed) -*/ 10.0f,
				       vel[1] - (spinmat[1] * crossvec[0] + spinmat[5] * crossvec[1] + spinmat[9] * crossvec[2]) + /*rsRandf2(20.0f,seed,&seed) -*/ 10.0f,
				       vel[2] - (spinmat[2] * crossvec[0] + spinmat[6] * crossvec[1] + spinmat[10] * crossvec[2]) + /*rsRandf2(20.0f,seed,&seed) -*/ 10.0f);
			newp->size = 15.0f;
			newp->rgb = priRGB;
			newp->makeSmoke = 0;
			newp->t = newp->tr = /*rsRandf2(0.5f,seed,&seed) + */1.5f;
		}
		tilt += temp;

		}
		break;

	case STREAMER: {
		// trail from streamers
		rsVec diff = xyz - lastxyz;

		// distance streamer traveled since last frame
		sparkTrailLength += diff.length ();
		// number of sparks to release each frame
		int sparks = int (sparkTrailLength * 0.04f);
		sparkTrailLength -= float (sparks) * 25.0f;

		for (i = 0; i < sparks/1.5; i++) {
			newp = addParticle(seed++);
			newp->initStar();
			newp->xyz = xyz - (diff * rsRandf(1.0f));
			newp->vel.set (vel[0] + rsRandf(80.0f) - 40.0f, vel[1] + rsRandf(80.0f) - 40.0f, vel[2] + rsRandf(80.0f) - 40.0f);
			newp->drag = 2.5f;
			newp->size = rsRandf(8.0f) + 4.0f;
			newp->rgb.set (1.0f, 0.8f, 0.6f);
			newp->t = rsRandf(2.0f) + 1.0f;
			newp->tr = newp->t;
			newp->makeSmoke = 0;
		}

		}
		break;

	case METEOR: {
		// trail from meteors
		rsVec diff = xyz - lastxyz;

		// distance rocket traveled since last frame
		sparkTrailLength += diff.length ();
		// number of smoke puffs to release
		int stars = int (sparkTrailLength * 0.1f + 0.5f);
		rsVec smkpos = lastxyz;

		// release star every 10 feet
		float multiplier = 10.0f / sparkTrailLength;

		for (i = 0; i < stars/1.5; i++) {
			smkpos += diff * multiplier;
			newp = addParticle(seed++);
			newp->initStar();
			newp->xyz = smkpos;
			newp->vel.set (vel[0] + rsRandf(40.0f) - 20.0f, vel[1] + rsRandf(40.0f) - 20.0f, vel[2] + rsRandf(40.0f) - 20.0f);
			newp->rgb = rgb;
			newp->drag = 2.0f;
			newp->t = newp->tr = rsRandf(0.5f) + 1.5f;
			newp->size = 10.0f;
			newp->makeSmoke = 0;
		}
		sparkTrailLength -= float (stars) * 10.0f;

		}
		break;

	case BEE: {
		// trail from bees
		rsVec diff = xyz - lastxyz;

		// distance rocket traveled since last frame
		sparkTrailLength += diff.length ();
		// number of smoke puffs to release
		int stars = int (sparkTrailLength * 0.1f + 0.5f);
		rsVec smkpos = lastxyz;

		// release star every 10 feet
		float multiplier = 10.0f / sparkTrailLength;

		for (i = 0; i < stars; i++) {
			smkpos += diff * multiplier;
			newp = addParticle(seed++);
			newp->initStar ();
			newp->xyz = smkpos;
			newp->vel.set (rsRandf(100.0f) - 50.0f - vel[0] * 0.5f, rsRandf(100.0f) - 50.0f - vel[1] * 0.5f, rsRandf(100.0f) - 50.0f - vel[2] * 0.5f);
			newp->rgb = rgb; // PMW BEE, trails of same color
			newp->t = newp->tr = rsRandf(0.1f) + 0.15f;
			newp->size = 7.0f;
			newp->displayList = flarelist[3];
			newp->makeSmoke = 0;
		}
		sparkTrailLength -= float (stars) * 10.0f;

		}
		break;

	case SUCKER:
		// pulling of particles by suckers
		pulling (this);
		break;

	case SHOCKWAVE:
		// pushing of particles by shockwaves
		pushing (this);
		break;

	case STRETCHER:
		// stretching of particles by stretchers
		stretching (this);
	}

// 	// smoke and cloud illumination from rockets and explosions
// 	if (dIllumination && ((type == ROCKET) || (type == FOUNTAIN) || (type == EXPLOSION)))
// 		illuminate (this);
}

/* TODO Dead code below, never called. To be removed. */
void particle::draw()
{
	if (life <= 0.0f)
		return;		// don't draw dead particles

	// cull small particles that are behind camera
	if (depth < 0.0f && type != SHOCKWAVE)
		return;

	// don't draw invisible particles
	if (type == POPPER)
		return;

	glPushMatrix ();
	glTranslatef (xyz[0], xyz[1], xyz[2]);

	if (type == SHOCKWAVE) {
		glScalef (size, size, size);
		drawShockwave (life, float (sqrt (size)) * 0.08f);

		if (life > 0.7f) {	// Big torus just for fun
			glMultMatrixf (billboardMat);
			glScalef (5.0f, 5.0f, 5.0f);
			glColor4f (1.0f, life, 1.0f, (life - 0.7f) * 3.333f);
// 			glCallList (flarelist[2]);
			


/*		
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(2, GL_FLOAT, 0, q3);
    glDrawArrays(GL_TRIANGLE_FAN,0,4);
    glDisableClientState(GL_VERTEX_ARRAY);
    */


    
      glBindTexture (GL_TEXTURE_2D, flaretex[2]);
      glEnable(GL_TEXTURE_2D);
		    
      glTexCoordPointer(2, GL_FLOAT, 0, tex1);
      glEnableClientState(GL_TEXTURE_COORD_ARRAY);

 
      glVertexPointer(3, GL_FLOAT, 0, vtx1);
      glEnableClientState(GL_VERTEX_ARRAY);
      
      
      glDrawArrays(GL_TRIANGLE_STRIP,0,4);
 
      glDisableClientState(GL_VERTEX_ARRAY);
      glDisableClientState(GL_TEXTURE_COORD_ARRAY);    
		
      glDisable(GL_TEXTURE_2D);
      
      
// 		glBegin (GL_TRIANGLE_STRIP);
// 		glTexCoord2f (0.0f, 0.0f);
// 		glVertex3f (-0.5f, -0.5f, 0.0f);
// 		glTexCoord2f (1.0f, 0.0f);
// 		glVertex3f (0.5f, -0.5f, 0.0f);
// 		glTexCoord2f (0.0f, 1.0f);
// 		glVertex3f (-0.5f, 0.5f, 0.0f);
// 		glTexCoord2f (1.0f, 1.0f);
// 		glVertex3f (0.5f, 0.5f, 0.0f);
// 		glEnd ();
		
		}
		glPopMatrix ();
		return;
	}

	glScalef (size, size, size);
	glMultMatrixf (billboardMat);
	if (type == SMOKE) {
		glColor4f (rgb[0], rgb[1], rgb[2], bright);
		glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
// 		glCallList (displayList);
		
		glBindTexture (GL_TEXTURE_2D, flaretex[displayList]);	

//       glEnableClientState(GL_VERTEX_ARRAY);
//       glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		
		
      glBindTexture (GL_TEXTURE_2D, flaretex[displayList]);
//       glEnable(GL_TEXTURE_2D);
		    
      glTexCoordPointer(2, GL_FLOAT, 0, tex1);
//       glEnableClientState(GL_TEXTURE_COORD_ARRAY);

 
      glVertexPointer(3, GL_FLOAT, 0, vtx1);
//       glEnableClientState(GL_VERTEX_ARRAY);
      
      
      glDrawArrays(GL_TRIANGLE_STRIP,0,4);
 
//       glDisableClientState(GL_VERTEX_ARRAY);
//       glDisableClientState(GL_TEXTURE_COORD_ARRAY);    
// 		
//       glDisable(GL_TEXTURE_2D);		
		
/* 
      glVertexPointer(3, GL_FLOAT, 0, vtx1);
      glTexCoordPointer(2, GL_FLOAT, 0, tex1);
      glDrawArrays(GL_TRIANGLE_STRIP,0,4);*/
 
//       glDisableClientState(GL_VERTEX_ARRAY);
//       glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		
/*		glBegin (GL_TRIANGLE_STRIP);
		glTexCoord2f (0.0f, 0.0f);
		glVertex3f (-0.5f, -0.5f, 0.0f);
		glTexCoord2f (1.0f, 0.0f);
		glVertex3f (0.5f, -0.5f, 0.0f);
		glTexCoord2f (0.0f, 1.0f);
		glVertex3f (-0.5f, 0.5f, 0.0f);
		glTexCoord2f (1.0f, 1.0f);
		glVertex3f (0.5f, 0.5f, 0.0f);
		glEnd ();*/		
		
	} 
	else {
		
          
          
          
          glBlendFunc (GL_SRC_ALPHA, GL_ONE);
		if (type == EXPLOSION) {
			glColor4f (1.0f, 1.0f, 1.0f, bright);
			glScalef (bright, bright, bright);
			
// 		glBindTexture (GL_TEXTURE_2D, flaretex[displayList]);		
		
		
//       glEnableClientState(GL_VERTEX_ARRAY);
//       glEnableClientState(GL_TEXTURE_COORD_ARRAY);
 
//       glVertexPointer(3, GL_FLOAT, 0, vtx1);
//       glTexCoordPointer(2, GL_FLOAT, 0, tex1);
//       glDrawArrays(GL_TRIANGLE_STRIP,0,4);
		
		
      glBindTexture (GL_TEXTURE_2D, flaretex[displayList]);
//       glEnable(GL_TEXTURE_2D);
		    
      glTexCoordPointer(2, GL_FLOAT, 0, tex1);
//       glEnableClientState(GL_TEXTURE_COORD_ARRAY);

 
      glVertexPointer(3, GL_FLOAT, 0, vtx1);
//       glEnableClientState(GL_VERTEX_ARRAY);
      
      
      glDrawArrays(GL_TRIANGLE_STRIP,0,4);
 
//       glDisableClientState(GL_VERTEX_ARRAY);
//       glDisableClientState(GL_TEXTURE_COORD_ARRAY);    
		
//       glDisable(GL_TEXTURE_2D);		
		
 
//       glDisableClientState(GL_VERTEX_ARRAY);
//       glDisableClientState(GL_TEXTURE_COORD_ARRAY);		
		
/*		glBegin (GL_TRIANGLE_STRIP);
		glTexCoord2f (0.0f, 0.0f);
		glVertex3f (-0.5f, -0.5f, 0.0f);
		glTexCoord2f (1.0f, 0.0f);
		glVertex3f (0.5f, -0.5f, 0.0f);
		glTexCoord2f (0.0f, 1.0f);
		glVertex3f (-0.5f, 0.5f, 0.0f);
		glTexCoord2f (1.0f, 1.0f);
		glVertex3f (0.5f, 0.5f, 0.0f);
		glEnd ();*/			
// 			glCallList (displayList);
		} else {
			glColor4f (rgb[0], rgb[1], rgb[2], bright);
			
// 		glBindTexture (GL_TEXTURE_2D, flaretex[displayList]);		
		
		
//       glEnableClientState(GL_VERTEX_ARRAY);
//       glEnableClientState(GL_TEXTURE_COORD_ARRAY);
 
//       glVertexPointer(3, GL_FLOAT, 0, vtx1);
//       glTexCoordPointer(2, GL_FLOAT, 0, tex1);
//       glDrawArrays(GL_TRIANGLE_STRIP,0,4);
 
//       glDisableClientState(GL_VERTEX_ARRAY);
//       glDisableClientState(GL_TEXTURE_COORD_ARRAY);	
			
      glBindTexture (GL_TEXTURE_2D, flaretex[displayList]);
//       glEnable(GL_TEXTURE_2D);
		    
      glTexCoordPointer(2, GL_FLOAT, 0, tex1);
//       glEnableClientState(GL_TEXTURE_COORD_ARRAY);

 
      glVertexPointer(3, GL_FLOAT, 0, vtx1);
//       glEnableClientState(GL_VERTEX_ARRAY);
      
      
      glDrawArrays(GL_TRIANGLE_STRIP,0,4);
 
//       glDisableClientState(GL_VERTEX_ARRAY);
//       glDisableClientState(GL_TEXTURE_COORD_ARRAY);    
		
//       glDisable(GL_TEXTURE_2D);			
			
		
/*		glBegin (GL_TRIANGLE_STRIP);
		glTexCoord2f (0.0f, 0.0f);
		glVertex3f (-0.5f, -0.5f, 0.0f);
		glTexCoord2f (1.0f, 0.0f);
		glVertex3f (0.5f, -0.5f, 0.0f);
		glTexCoord2f (0.0f, 1.0f);
		glVertex3f (-0.5f, 0.5f, 0.0f);
		glTexCoord2f (1.0f, 1.0f);
		glVertex3f (0.5f, 0.5f, 0.0f);
		glEnd ();*/			
// 			glCallList (displayList);
			glScalef (0.35f, 0.35f, 0.35f);
			glColor4f (1.0f, 1.0f, 1.0f, bright);
			
// 		glBindTexture (GL_TEXTURE_2D, flaretex[displayList]);
		
//       glEnableClientState(GL_VERTEX_ARRAY);
//       glEnableClientState(GL_TEXTURE_COORD_ARRAY);
 
//       glVertexPointer(3, GL_FLOAT, 0, vtx1);
//       glTexCoordPointer(2, GL_FLOAT, 0, tex1);
//       glDrawArrays(GL_TRIANGLE_STRIP,0,4);
			
			
      glBindTexture (GL_TEXTURE_2D, flaretex[displayList]);
//       glEnable(GL_TEXTURE_2D);
		    
      glTexCoordPointer(2, GL_FLOAT, 0, tex1);
//       glEnableClientState(GL_TEXTURE_COORD_ARRAY);

 
      glVertexPointer(3, GL_FLOAT, 0, vtx1);
//       glEnableClientState(GL_VERTEX_ARRAY);
      
      
      glDrawArrays(GL_TRIANGLE_STRIP,0,4);
 
//       glDisableClientState(GL_VERTEX_ARRAY);
//       glDisableClientState(GL_TEXTURE_COORD_ARRAY);    
		
//       glDisable(GL_TEXTURE_2D);				
			
 
//       glDisableClientState(GL_VERTEX_ARRAY); 
//       glDisableClientState(GL_TEXTURE_COORD_ARRAY);		
		
/*		glBegin (GL_TRIANGLE_STRIP);
		glTexCoord2f (0.0f, 0.0f);
		glVertex3f (-0.5f, -0.5f, 0.0f);
		glTexCoord2f (1.0f, 0.0f);
		glVertex3f (0.5f, -0.5f, 0.0f);
		glTexCoord2f (0.0f, 1.0f);
		glVertex3f (-0.5f, 0.5f, 0.0f);
		glTexCoord2f (1.0f, 1.0f);
		glVertex3f (0.5f, 0.5f, 0.0f);
		glEnd ();*/			
// 			glCallList (displayList);
		}
	}

	glPopMatrix ();
}
