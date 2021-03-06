/*

 IPlug distortion example
 (c) Theo Niessink 2011
 <http://www.taletn.com/>

 This software is provided 'as-is', without any express or implied
 warranty. In no event will the authors be held liable for any damages
 arising from the use of this software.

 Permission is granted to anyone to use this software for any purpose,
 including commercial applications, and to alter it and redistribute it
 freely, subject to the following restrictions:

 1. The origin of this software must not be misrepresented; you must not
 claim that you wrote the original software. If you use this software in a
 product, an acknowledgment in the product documentation would be
 appreciated but is not required.
 2. Altered source versions must be plainly marked as such, and must not be
 misrepresented as being the original software.
 3. This notice may not be removed or altered from any source distribution.


 Simple IPlug audio effect that shows how to implement oversampling to reduce
 aliasing.

 */


#include "FreakkTrem.h"
#include "IPlug_include_in_plug_src.h"
//#include "IAutoGUI.h"
#include "IControl.h"

#include <math.h>
#include "../../WDL/denormal.h"


FreakkTrem::FreakkTrem(IPlugInstanceInfo instanceInfo):
  IPLUG_CTOR(kNumParams, 0, instanceInfo)
{
  TRACE;
  //arguments are: name, defaultVal, minVal, maxVal, step, label
  GetParam(kGain)->InitDouble("Gain", 1, 0., 1.5, 0.001, ""); // Gain knob offers a little amplification
  GetParam(kDepth)->InitDouble("Depth", 0.5, 0., 1., 0.001, "%");
  GetParam(kFreq)->InitDouble("Freq", 6, 0., 12, 0.5, "Hz");
  GetParam(kModMix)->InitDouble("Mod", 0., 0., 1., 0.01, "S<>C" ); // Mod Mixer
  

  GetParam(kIKnobRotaterControl_def)->InitDouble("IKnobRotaterControl Default", 2., .0, 5., .1, "");

  IGraphics* pGraphics = MakeGraphics(this, kW, kH); // MakeGraphics(this, kW, kH);
  pGraphics->AttachBackground(BG_ID, BG_FN);
  
  //Attach the IKnobRotaterControl default rotation range
  IBitmap bitmap = pGraphics->LoadIBitmap(KNOB_SPEED_ID, KNOB_SPEED_FN);
  pGraphics->AttachControl(new IKnobRotaterControl(this, kSpeedKnob_def_X, kSpeedKnob_def_Y, kFreq, &bitmap));
  
  bitmap = pGraphics->LoadIBitmap(KNOB_DEPTH_ID, KNOB_DEPTH_FN);
  pGraphics->AttachControl(new IKnobRotaterControl(this, kDepthKnob_def_X, kDepthKnob_def_Y, kDepth, &bitmap));

  bitmap = pGraphics->LoadIBitmap(KNOB_GAIN_ID, KNOB_GAIN_FN);
  pGraphics->AttachControl(new IKnobRotaterControl(this, kGainKnob_def_X, kGainKnob_def_Y, kGain, &bitmap));

  bitmap = pGraphics->LoadIBitmap(KNOB_TYPE_ID, KNOB_TYPE_FN);
  pGraphics->AttachControl(new IKnobRotaterControl(this, kTypeKnob_def_X, kTypeKnob_def_Y, kModMix, &bitmap));

  //IGraphics* pGraphics = MakeGraphics(this, GUI_WIDTH, GUI_HEIGHT);
  //IText textProps(12, &COLOR_BLACK, "Verdana", IText::kStyleNormal, IText::kAlignNear, 0, IText::kQualityNonAntiAliased);
  //GenerateKnobGUI(pGraphics, this, &textProps, &COLOR_WHITE, &COLOR_BLACK, 60, 70);

  AttachGraphics(pGraphics);
  panL = panR = 1;

  //mSampleRate = GetSampleRate();
  counter=0;

}


void FreakkTrem::OnParamChange(int paramIdx)
{
  IMutexLock lock(this);

  switch (paramIdx)
  {
    case kGain:
	  mGain = GetParam(kGain)->Value();
	  break;
	case kDepth:
	  mDepth = GetParam(kDepth)->Value();
	  break;
	case kFreq:
	  mFreq = GetParam(kFreq)->Value(); // divided by 2 gives the Peak Frequency
	  break;
	case kModMix:
	  mModMix = (int)GetParam(kModMix)->Value();
	  break;
	default:
	  break;
  }
}


void FreakkTrem::ProcessDoubleReplacing(double** inputs, double** outputs, int sampleFrames)
{
	double* in1 = inputs[0];
	double* in2 = inputs[1];
	double* out1 = outputs[0];
	double* out2 = outputs[1];

	bool isMono = !IsInChannelConnected(1);
	
	double samplerate = GetSampleRate();
	double sineMod, squareMod;
	int window = 0;
	
	for (int s = 0; s < sampleFrames; ++s, ++in1, ++in2, ++out1, ++out2)
	{
	
	// increment our counter
	if(counter < ((1/mFreq) * samplerate)) // counter goes from 0 to T = 1/f
	{
	 counter++;
	}
	else
	{
	counter = 0;
	}
		// f = counter/mSampleRate
		// T = 1/f = mSampleRate/counter
		// calculate SINE multiplier: 
		// 1) SINE WAVE CARRIER:
		// v(t) = (Ec + em) * sin(2*PI*Freq*t) ==> [ Ec + Em * sin(2*PI*Freq*t) ] sin(2*PI*Freq*t) 
		// Ec = carrier wave
		// em = Em*sin(2*PI*Freq*t) instantaneous amplitude of the modulating signal
		// panL = panR = 1;
		
			// A
			sineMod = (1.0 - mDepth) + mDepth * ( sin((2 * 3.14 *(mFreq/2)) * (counter/samplerate)) ) * (sin((2 *3.14 * (mFreq/2)) * (counter/samplerate)));
			// B
			//sineMod = (1.0 - mDepth) + mDepth *  ( 0.5 + 0.5 * sin((2 *3.14 * (mFreq)) * (counter/samplerate)));
			squareMod = (1.0 - mDepth) + mDepth *  ( 0.5 + 0.5 * tanh( 10 * sin((2 *3.14 * (mFreq)) * (counter/samplerate)) ) );
			*out1 = *in1 * ((1-mModMix) * sineMod + mModMix * squareMod) * mGain * panL;
			*out2 = *in2 * ((1-mModMix) * sineMod + mModMix * squareMod) * mGain * panR;	

		
	}
	/*	
	for (int s = 0; s < sampleFrames; ++s, ++in1, ++in2, ++out1, ++out2)
	  {
		*out1 = *in1 * mGain;
		*out2 = *in2 * mGain;
	  }
	*/
}
