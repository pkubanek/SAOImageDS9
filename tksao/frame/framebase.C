// Copyright (C) 1999-2018
// Smithsonian Astrophysical Observatory, Cambridge, MA, USA
// For conditions of distribution and use, see copyright notice in "copyright"

#include <tkInt.h>

#include "framebase.h"
#include "fitsimage.h"
#include "marker.h"
#include "context.h"
#include "ps.h"
#include "sigbus.h"

// Public

FrameBase::FrameBase(Tcl_Interp* i, Tk_Canvas c, Tk_Item* item) 
  : Base(i, c, item)
{
  rotateSrcXM = NULL;
  rotateDestXM = NULL;
  rotatePM = 0;

  colormapXM = NULL;
  colormapPM = 0;
  colormapGCXOR = 0;
}

FrameBase::~FrameBase()
{
  if (colormapXM)
    XDestroyImage(colormapXM);

  if (colormapPM)
    Tk_FreePixmap(display, colormapPM);

  if (colormapGCXOR)
    XFreeGC(display, colormapGCXOR);
}

void FrameBase::getInfoCmd(const Vector& vv, Coord::InternalSystem ref,
			   char* var)
{
  FitsBound* params;
  int mosaic;

  Vector rr = mapToRef(vv,ref);

  // make sure we have an image
  FitsImage* ptr = currentContext->cfits;
  FitsImage* sptr = currentContext->cfits;
  if (!ptr)
    goto noFits;

  mosaic = isMosaic();
  params = sptr->getDataParams(currentContext->secMode());

  if (!mosaic) {
    Tcl_SetVar2(interp,var,"filename",(char*)sptr->getFileName(ROOTBASE),0);
    Tcl_SetVar2(interp,var,"object",(char*)sptr->objectKeyword(),0);
    Tcl_SetVar2(interp,var,"min",(char*)sptr->getMin(),0);
    Tcl_SetVar2(interp,var,"min,x",(char*)sptr->getMinX(),0);
    Tcl_SetVar2(interp,var,"min,y",(char*)sptr->getMinY(),0);
    Tcl_SetVar2(interp,var,"max",(char*)sptr->getMax(),0);
    Tcl_SetVar2(interp,var,"max,x",(char*)sptr->getMaxX(),0);
    Tcl_SetVar2(interp,var,"max,y",(char*)sptr->getMaxY(),0);
    Tcl_SetVar2(interp,var,"low",(char*)sptr->getLow(),0);
    Tcl_SetVar2(interp,var,"high",(char*)sptr->getHigh(),0);
  }

  if (((Vector&)vv)[0]<0 && ((Vector&)vv)[1]<0)
    goto noImage;

  // clear values
  Tcl_SetVar2(interp,var,"value","",0);
  Tcl_SetVar2(interp,var,"value,red","",0);
  Tcl_SetVar2(interp,var,"value,green","",0);
  Tcl_SetVar2(interp,var,"value,blue","",0);

  do {
    Vector img = rr * sptr->refToData;

    if (img[0]>=params->xmin && img[0]<params->xmax && 
	img[1]>=params->ymin && img[1]<params->ymax) {

      if (mosaic) {
	Tcl_SetVar2(interp,var,"filename",(char*)sptr->getFileName(ROOTBASE),0);
	Tcl_SetVar2(interp,var,"object",(char*)sptr->objectKeyword(),0);
	Tcl_SetVar2(interp,var,"min",(char*)sptr->getMin(),0);
	Tcl_SetVar2(interp,var,"min,x",(char*)sptr->getMinX(),0);
	Tcl_SetVar2(interp,var,"min,y",(char*)sptr->getMinY(),0);
	Tcl_SetVar2(interp,var,"max",(char*)sptr->getMax(),0);
	Tcl_SetVar2(interp,var,"max,x",(char*)sptr->getMaxX(),0);
	Tcl_SetVar2(interp,var,"max,y",(char*)sptr->getMaxY(),0);
	Tcl_SetVar2(interp,var,"low",(char*)sptr->getLow(),0);
	Tcl_SetVar2(interp,var,"high",(char*)sptr->getHigh(),0);
      }

      SETSIGBUS
      Tcl_SetVar2(interp,var,"value",(char*)sptr->getValue(img),0);
      CLEARSIGBUS

      coordToTclArray(sptr,rr,Coord::IMAGE,var,"image");
      coordToTclArray(sptr,rr,Coord::PHYSICAL,var,"physical");
      if (hasATMV())
	coordToTclArray(sptr,rr,Coord::AMPLIFIER,var,"amplifier");
      if (hasDTMV())
	coordToTclArray(sptr,rr,Coord::DETECTOR,var,"detector");

      getInfoWCS(var,rr,ptr,sptr);
      return;
    }
    else {
      if (mosaic) {
	sptr = sptr->nextMosaic();
	if (sptr)
	  params = sptr->getDataParams(currentContext->secMode());
      }
      else {
	getInfoWCS(var,rr,ptr,sptr);
	goto noImage;
      }
    }
  }
  while (mosaic && sptr);

  // mosaic gap
  getInfoWCS(var,rr,ptr,ptr);

  // else, return blanks
 noFits:
  getInfoClearName(var);

 noImage:
  getInfoClearValue(var);
}

void FrameBase::getInfoWCS(char* var, Vector& rr, FitsImage* ptr, 
			   FitsImage* sptr)
{
  Vector img = rr * sptr->refToImage;

  for (int ii=0; ii<MULTWCS; ii++) {
    char buf[64];
    char ww = !ii ? '\0' : '`'+ii;
    Coord::CoordSystem www = (Coord::CoordSystem)(Coord::WCS+ii);

    if (hasWCS(www)) {
      char buff[128];
      sptr->pix2wcs(img, www, wcsSky_, wcsSkyFormat_, buff);

      int argc;
      const char** argv;
      Tcl_SplitList(interp, buff, &argc, &argv);

      if (argc > 0 && argv && argv[0])
	Tcl_SetVar2(interp,var,varcat(buf,(char*)"wcs",ww,(char*)",x"),argv[0],0);
      else
	Tcl_SetVar2(interp,var,varcat(buf,(char*)"wcs",ww,(char*)",x"),"",0);
      if (argc > 1 && argv && argv[1])
	Tcl_SetVar2(interp,var,varcat(buf,(char*)"wcs",ww,(char*)",y"),argv[1],0);
      else
	Tcl_SetVar2(interp,var,varcat(buf,(char*)"wcs",ww,(char*)",y"),"",0);

      char* wcsname = (char*)sptr->getWCSName(www);
      if (wcsname)
	Tcl_SetVar2(interp,var,varcat(buf,(char*)"wcs",ww,(char*)",sys"),wcsname,0);
      else if (argc > 2 && argv && argv[2])
	Tcl_SetVar2(interp,var,varcat(buf,(char*)"wcs",ww,(char*)",sys"),argv[2],0);
      else
	Tcl_SetVar2(interp,var,varcat(buf,(char*)"wcs",ww,(char*)",sys"),"",0);
	    
      Tcl_Free((char*)argv);
    }
    else {
      Tcl_SetVar2(interp,var,varcat(buf,(char*)"wcs",ww,(char*)",x"),"",0);
      Tcl_SetVar2(interp,var,varcat(buf,(char*)"wcs",ww,(char*)",y"),"",0);
      Tcl_SetVar2(interp,var,varcat(buf,(char*)"wcs",ww,(char*)",z"),"",0);
      Tcl_SetVar2(interp,var,varcat(buf,(char*)"wcs",ww,(char*)",sys"),"",0);
    }
  }
}

void FrameBase::coordToTclArray(FitsImage* ptr, const Vector& vv, 
				Coord::CoordSystem out,
				const char* var, const char* base)
{
  Vector rr = ptr->mapFromRef(vv, out);
  doubleToTclArray(rr[0], var, base, "x");
  doubleToTclArray(rr[1], var, base, "y");
}

double FrameBase::calcZoomPanner()
{
  if (!(keyContext->fits && pannerPixmap))
    return 1;

  Vector src = imageSize(keyContext->datasec() ? FrScale::DATASEC : FrScale::IMGSEC);
  return calcZoom(src, Vector(pannerWidth,pannerHeight));
}

void FrameBase::rotateMotion()
{
  // Rotate from src to dest
  Vector cc = Vector(options->width,options->height)/2;
  Matrix m = (Translate(-cc) * Rotate(rotation-rotateRotation) * 
	      Translate(cc)).invert();
  double* mm = m.mm();

  int& width = options->width;
  int& height = options->height;
  char* src = rotateSrcXM->data;

  int bytesPerPixel = rotateDestXM->bits_per_pixel/8;

  for (int j=0; j<height; j++) {
    // the line may be padded at the end
    char* dest = rotateDestXM->data + j*rotateDestXM->bytes_per_line;
    memset(dest,0,rotateDestXM->bytes_per_line);

    for (int i=0; i<width; i++, dest+=bytesPerPixel) {
      double x = i*mm[0] + j*mm[3] + mm[6];
      double y = i*mm[1] + j*mm[4] + mm[7];

      if (x >= 0 && x < width && y >= 0 && y < height) {
#if MAC_OSX_TK
	// I really don't understand this
	char* sptr = src + ((int)y)*rotateDestXM->bytes_per_line+
	  ((int)x)*bytesPerPixel;

	*(dest+0) = *(sptr+3);
	*(dest+1) = *(sptr+0);
	*(dest+2) = *(sptr+1);
	*(dest+3) = *(sptr+2);
#else
	memcpy(dest, src + ((int)y)*rotateDestXM->bytes_per_line +
	  ((int)x)*bytesPerPixel, bytesPerPixel);
#endif
      }
      else
	memcpy(dest, bgTrueColor_, bytesPerPixel);
    }
  }

  // XImage to Pixmap
  TkPutImage(NULL, 0, display, rotatePM, widgetGC, rotateDestXM,
	    0, 0, 0, 0, options->width, options->height);

  // Display Pixmap
  Vector dd = Vector() * widgetToWindow;
  XCopyArea(display, rotatePM, Tk_WindowId(tkwin), rotateGCXOR, 0, 0, 
	    options->width, options->height, dd[0], dd[1]);
}
void FrameBase::setBinCursor()
{
  if (context->cfits)
    context->cfits->setBinCursor(cursor);
}

void FrameBase::setSlice(int id, int ss)
{
  // IMAGE (ranges 1-n)
  currentContext->updateSlice(id, ss);

  switch (currentContext->clipScope()) {
  case FrScale::GLOBAL:
    break;
  case FrScale::LOCAL:
    currentContext->clearHist();
    currentContext->updateClip();
    break;
  }

  currentContext->updateContours();
  updateColorScale();
  update(MATRIX);

  Base::setSlice(id,ss);
}

void FrameBase::updateBin(const Matrix& mx)
{
  // Note: cursor is in REF coords, imageCenter() in IMAGE coords
  cursor = imageCenter(FrScale::IMGSEC);
  Base::updateBin(mx);
}

#ifndef NEWWCS
void FrameBase::updatePanner()
{
  Base::updatePanner();

  if (usePanner) {
    ostringstream str;

    str << pannerName << " update " << (void*)pannerPixmap << ';';

    // calculate bbox
    Vector ll = Vector(0,0) * widgetToPanner;
    Vector lr = Vector(options->width,0) * widgetToPanner;
    Vector ur = Vector(options->width,options->height) * widgetToPanner;
    Vector ul = Vector(0,options->height) * widgetToPanner;

    str << pannerName << " update bbox " 
	<< ll << ' ' << lr << ' ' << ur << ' ' << ul << ';';

    // calculate image compass vectors
    Matrix mm = 
      FlipY() *
      irafMatrix_ *
      wcsOrientationMatrix * 
      Rotate(wcsRotation) *
      orientationMatrix *
      Rotate(rotation);

    Vector xx = (Vector(1,0)*mm).normalize();
    Vector yy = (Vector(0,1)*mm).normalize();

    str << pannerName << " update image compass " 
	<< xx << ' ' << yy << ';';

    if (keyContext->fits && keyContext->fits->hasWCS(wcsSystem_)) {
      Vector orpix = keyContext->fits->center();
      Vector orval=keyContext->fits->pix2wcs(orpix, wcsSystem_, wcsSky_);
      Vector orpix2 = keyContext->fits->wcs2pix(orval, wcsSystem_,wcsSky_);
      Vector delta = keyContext->fits->getWCScdelt(wcsSystem_).abs();

      Vector npix = keyContext->fits->wcs2pix(Vector(orval[0],orval[1]+delta[1]), wcsSystem_,wcsSky_);
      Vector north = ((npix-orpix2)*mm).normalize();
      Vector epix = keyContext->fits->wcs2pix(Vector(orval[0]+delta[0],orval[1]), wcsSystem_,wcsSky_);
      Vector east = ((epix-orpix2)*mm).normalize();

      // sanity check
      Vector diff = (north-east).abs();
      if ((north[0]==0 && north[1]==0) ||
	  (east[0]==0 && east[1]==0) ||
	  (diff[0]<.01 && diff[1]<.01)) {
	north = (Vector(0,1)*mm).normalize();
	east = (Vector(-1,0)*mm).normalize();
      }

      str << pannerName << " update wcs compass " 
	  << north << ' ' << east << ends;
    }
    else
      str << pannerName << " update wcs compass invalid" << ends;

    Tcl_Eval(interp, str.str().c_str());
  }
}
#else
void FrameBase::updatePanner()
{
  Base::updatePanner();

  if (usePanner) {
    ostringstream str;

    str << pannerName << " update " << (void*)pannerPixmap << ';';

    // calculate bbox
    Vector ll = Vector(0,0) * widgetToPanner;
    Vector lr = Vector(options->width,0) * widgetToPanner;
    Vector ur = Vector(options->width,options->height) * widgetToPanner;
    Vector ul = Vector(0,options->height) * widgetToPanner;

    str << pannerName << " update bbox " 
	<< ll << ' ' << lr << ' ' << ur << ' ' << ul << ';';

    // calculate image compass vectors
    Matrix mm = 
      FlipY() *
      irafMatrix_ *
      wcsOrientationMatrix * 
      Rotate(wcsRotation) *
      orientationMatrix *
      Rotate(rotation);

    Vector xx = (Vector(1,0)*mm).normalize();
    Vector yy = (Vector(0,1)*mm).normalize();

    str << pannerName << " update image compass " 
	<< xx << ' ' << yy << ';';

    if (keyContext->fits && keyContext->fits->hasWCS(wcsSystem_)) {
      Matrix mx;
      Coord::Orientation oo = 
	keyContext->fits->getWCSOrientation(wcsSystem_, wcsSky_);
      if (hasWCSCel(wcsSystem_)) {
	if (oo==Coord::XX)
	  mx *= FlipX();
      }
      else {
	if (oo==Coord::NORMAL)
	  mx *= FlipX();
      }

      double rr = keyContext->fits->getWCSRotation(wcsSystem_, wcsSky_);
      mx *= Rotate(rr)*mm;
      Vector north = (Vector(0,1)*mx).normalize();
      Vector east = (Vector(-1,0)*mx).normalize();

      str << pannerName << " update wcs compass " 
	  << north << ' ' << east << ends;
    }
    else
      str << pannerName << " update wcs compass invalid" << ends;

    Tcl_Eval(interp, str.str().c_str());
  }
}
#endif

void FrameBase::x11MagnifierCursor(const Vector& vv)
{
  // first, the inner color'd box
  Vector uu = vv * canvasToUser;

  Matrix mm = Translate(-uu) * 
    Rotate(wcsRotation) *
    Rotate(rotation) *
    Scale(zoom_) *
    Scale(magnifierZoom_) *
    Translate(magnifierWidth/2., magnifierHeight/2.);

      Vector c[5];
    c[0] = (uu + Vector(-.5,-.5)) * mm;
    c[1] = (uu + Vector( .5,-.5)) * mm;
    c[2] = (uu + Vector( .5, .5)) * mm;
    c[3] = (uu + Vector(-.5, .5)) * mm;
    c[4] = c[0];
    
    XPoint pts[5];
    for (int ii=0; ii<5; ii++) {
      Vector z = c[ii].round();
      pts[ii].x = (short)z[0];
      pts[ii].y = (short)z[1];
    }
    XSetForeground(display, widgetGC, getColor(magnifierColorName));
    XDrawLines(display, magnifierPixmap, widgetGC, pts, 5, CoordModeOrigin);

    // now, the outer black box
    Matrix nn =
      Translate(-(uu * mm)) *
      Rotate(-rotation) *
      Rotate(-wcsRotation);
    Matrix oo = nn.invert();

    Vector dd[5];
    for (int ii=0; ii<5; ii++)
      dd[ii] = c[ii] * nn;

    dd[0]+=Vector(-1,-1);
    dd[1]+=Vector( 1,-1);
    dd[2]+=Vector( 1, 1);
    dd[3]+=Vector(-1, 1);
    dd[4]=dd[0];

    for (int ii=0; ii<5; ii++) {
      dd[ii] = dd[ii] * oo;
      Vector zz = dd[ii].round();
      pts[ii].x = (int)zz[0];
      pts[ii].y = (int)zz[1];
    }
    XSetForeground(display, widgetGC, getColor("black"));
    XDrawLines(display, magnifierPixmap, widgetGC, pts, 5, CoordModeOrigin);
}

