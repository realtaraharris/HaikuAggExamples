/*
 * Copyright 2023, Tara Harris <3769985+realtaraharris@users.noreply.github.com>
 * Portions copyright 2002-2006 Maxim Shemanarev
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#include "CirclesView.h"

static double splineRX[] = { 0.000000, 0.200000, 0.400000, 0.910484, 0.957258, 1.000000 };
static double splineRY[] = { 1.000000, 0.800000, 0.600000, 0.066667, 0.169697, 0.600000 };

static double splineGX[] = { 0.000000, 0.292244, 0.485655, 0.564859, 0.795607, 1.000000 };
static double splineGY[] = { 0.000000, 0.607260, 0.964065, 0.892558, 0.435571, 0.000000 };

static double splineBX[] = { 0.000000, 0.055045, 0.143034, 0.433082, 0.764859, 1.000000 };
static double splineBY[] = { 0.385480, 0.128493, 0.021416, 0.271507, 0.713974, 1.000000 };

double randomDouble(double start, double end) {
    unsigned r = rand() & 0x7FFF;
    return double(r) * (end - start) / 32768.0 + start;
}

void attachBufferToBBitmap(agg::rendering_buffer& buffer, BBitmap* bitmap) {
    uint8* bits = (uint8*)bitmap->Bits();
    uint32 width = bitmap->Bounds().IntegerWidth() + 1;
    uint32 height = bitmap->Bounds().IntegerHeight() + 1;
    int32 bpr = bitmap->BytesPerRow();
    buffer.attach(bits, width, height, -bpr);
}

CirclesView::CirclesView(BRect rect) :
	BView(rect, "Circles View", B_FOLLOW_ALL_SIDES, B_FRAME_EVENTS | B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE),
	pointCount(10000),
	circleDiameter(20),
	selectivity(0),
	z1(1),
	z2(1) {
    scatterPoints = new ScatterPoint[pointCount];

    splineR.init(6, splineRX, splineRY);
    splineG.init(6, splineGX, splineGY);
    splineB.init(6, splineBX, splineBY);

    initialRect = rect;
    currentRect = rect;

    SetTransAffineResizingMatrix(rect.Width(), rect.Height(), true);
    GeneratePoints();

    InitBitmapAndBuffer();
}

CirclesView::~CirclesView() {
    delete []scatterPoints;
}

void CirclesView::InitBitmapAndBuffer() {
    retainedBitmap = new BBitmap(currentRect, 0, B_RGBA32);
    if (retainedBitmap->IsValid()) {
        attachBufferToBBitmap(buffer, retainedBitmap);
    } else {
        delete retainedBitmap;
        retainedBitmap = NULL;
    }
}

//void CirclesView::AttachedToWindow() {}

//void CirclesView::DetachedFromWindow() {}

void CirclesView::Draw(BRect updateRect) {
    RenderCircles(updateRect);
    DrawBitmap(retainedBitmap, updateRect, updateRect);
}

// this is never actually called in this example because nothing moves the view frame
void CirclesView::FrameMoved(BPoint newLocation) {
    currentRect.SetLeftTop(newLocation);
}

void CirclesView::FrameResized(float width, float height) {
    currentRect.SetRightBottom(initialRect.LeftTop() + BPoint(width, height));
    SetTransAffineResizingMatrix(width + 1, height + 1, true);
    InitBitmapAndBuffer(); // do this before drawing
    Draw(currentRect);
}

void CirclesView::GeneratePoints() {
    double width = initialRect.Width();
    double height = initialRect.Height();
    double rx = width / 3.5;
    double ry = height / 3.5;

    for (unsigned i = 0; i < pointCount; i++) {
        double z = scatterPoints[i].z = randomDouble(0.0, 1.0);
        double x = cos(z * 2.0 * agg::pi) * rx;
        double y = sin(z * 2.0 * agg::pi) * ry;

        double dist  = randomDouble(0.0, rx / 2.0);
        double angle = randomDouble(0.0, agg::pi * 2.0);

        scatterPoints[i].x = width / 2.0  + x + cos(angle) * dist;
        scatterPoints[i].y = height / 2.0 + y + sin(angle) * dist;
        scatterPoints[i].color = agg::rgba(splineR.get(z) * 0.8,
                                      splineG.get(z) * 0.8,
                                      splineB.get(z) * 0.8,
                                      1.0);
    }
}

void CirclesView::SetTransAffineResizingMatrix(unsigned width, unsigned height, bool keepAspectRatio) {
    if (keepAspectRatio) {
        agg::trans_viewport vp;
        vp.preserve_aspect_ratio(0.5, 0.5, agg::aspect_ratio_meet);
        vp.device_viewport(0, 0, width, height);
        vp.world_viewport(0, 0, initialRect.Width(), initialRect.Height());
        resizeMatrix = vp.to_affine();
    } else {
        resizeMatrix = agg::trans_affine_scaling(
            width / initialRect.Width(),
            height / initialRect.Height());
    }
}

const agg::trans_affine& CirclesView::GetTransAffineResizingMatrix() const {
    return resizeMatrix;
}

void CirclesView::ChangePointCount(int delta) {
	if (pointCount > 0) {
		delete []scatterPoints;
	}

	const int newPointCount = pointCount + delta;
	if (newPointCount > 0) {
		pointCount = newPointCount;
		scatterPoints = new ScatterPoint[pointCount];
		GeneratePoints();
	} else {
		pointCount = 0;
	}
}

void CirclesView::RenderCircles(BRect rect) {
    agg::rasterizer_scanline_aa<> pf;
    agg::scanline_p8 sl;

    typedef agg::pixfmt_bgra32 pixel_format_type;
    typedef agg::renderer_base<pixel_format_type> renderer_base;

    pixel_format_type pixf(buffer);
    renderer_base rb(pixf);

    rb.clear(agg::rgba(1.0, 1.0, 1.0));

    agg::ellipse e1;

    const agg::trans_affine resize_mtx = GetTransAffineResizingMatrix();
    agg::conv_transform<agg::ellipse> t1(e1, resize_mtx);

    unsigned circlesDrawn = 0;
    for (unsigned i = 0; i < pointCount; i++) {
        double z = scatterPoints[i].z;
        double alpha = 1.0;
        if (z < z1) {
            alpha = 1.0 - (z1 - z) * (float(selectivity) / 10);
        }

        if (z > z2) {
            alpha = 1.0 - (z - z2) * (float(selectivity) / 10);
        }

        if (alpha > 1.0) alpha = 1.0;
        if (alpha < 0.0) alpha = 0.0;

        if (alpha > 0.0) {
            e1.init(scatterPoints[i].x,
                    scatterPoints[i].y,
                    (float)circleDiameter / 20,
                    (float)circleDiameter / 20,
                    8);
            pf.add_path(t1);

            agg::render_scanlines_aa_solid(
                pf, sl, rb,
                agg::rgba(scatterPoints[i].color.r,
                          scatterPoints[i].color.g,
                          scatterPoints[i].color.b,
                          alpha));
            circlesDrawn++;
        }
    }

    char buf[10];
    sprintf(buf, "%08u", circlesDrawn);

    agg::gsv_text text;
    text.size(15.0);
    text.text(buf);
    text.start_point(10.0, initialRect.Height() - 20.0);
    agg::gsv_text_outline<> textOutline(text, GetTransAffineResizingMatrix());
    pf.add_path(textOutline);
    agg::render_scanlines_aa_solid(pf, sl, rb, agg::rgba(0, 0, 0));
}

void CirclesView::SaveToPng() {
	writePng(&buffer, "CirclesOutput.png");
}
