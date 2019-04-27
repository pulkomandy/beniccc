/*
 * main.cpp
 * Copyright (C) 2019 pulkomandy <pulkomandy@pulkomandy.tk>
 *
 * Distributed under terms of the MIT license.
 */

#include <Alert.h>
#include <Application.h>
#include <BufferIO.h>
#include <DurationFormat.h>
#include <File.h>
#include <FileGameSound.h>
#include <GroupView.h>
#include <Polygon.h>
#include <Screen.h>
#include <String.h>
#include <Window.h>

#include <arpa/inet.h>
#include <stdio.h>

bool gBenchmark;

class DemoView: public BGroupView
{
	public:
		DemoView()
			: fDataStream(new BFile("scene1.bin", B_READ_ONLY))
			, fFrame(0)
		{
			for (int i = 0; i < 16; i++)
				fColor[i] = make_color(0, 0, 0, 255);
			SetLowColor(make_color(0, 0, 0, 255));
			if (gBenchmark)
				SetFlags(Flags() | B_WILL_DRAW);
			else
				SetFlags(Flags() | B_WILL_DRAW | B_PULSE_NEEDED);
		}

		uint8_t decode(uint8_t raw)
		{
			raw &= 0xF;
			uint8_t bits = (raw >> 3) & 1;
			bits |= (raw << 1) & 0xE;
			return bits;
		}

		void Draw(BRect r)
		{
			if (!gBenchmark) {
				BRect b = Bounds();
				SetScale(b.Height() / 200 + 1);
			}

			bool align = false;
			uint8_t flags;
			fDataStream.Read(&flags, 1);

			if (flags & 2) {
				// Palette
				uint16_t colors;
				fDataStream.Read(&colors, 2);
				colors = ntohs(colors);
				for (int i = 0; i < 16; i++) {
					if (colors & (1 << (15 - i))) {
						uint16_t color;
						fDataStream.Read(&color, 2);
						fColor[i].red = decode(color) * 17;
						fColor[i].blue = decode(color >> 8) * 17;
						fColor[i].green = decode(color >> 12) * 17;
					}
				}
			}

			if (flags & 1) {
				SetHighColor(fColor[0]);
				FillRect(Bounds());
			}

			if (flags & 4) {
				// Indexed mode
				uint8_t vertices;
				BPoint points[256];

				fDataStream.Read(&vertices, 1);
				for (int i = 0; i < vertices; i++)
				{
					uint8_t x,y;
					fDataStream.Read(&x, 1);
					fDataStream.Read(&y, 1);
					points[i].x = x;
					points[i].y = y;
				}
				for (;;)
				{
					uint8_t bits;
					fDataStream.Read(&bits, 1);
					if (bits == 0xFF) {
						align = false;
						break;
					} else if (bits == 0xFE) {
						align = true;
						break;
					} else if (bits == 0xFD) {
						be_app->PostMessage(B_QUIT_REQUESTED);
						break;
					} else {
						BPolygon p;
						for (int i = 0; i < (bits & 0xF); i++) {
							uint8_t index;
							fDataStream.Read(&index, 1);
							p.AddPoints(&points[index], 1);
						}
						SetHighColor(fColor[bits >> 4]);
						SetViewColor(fColor[bits >> 4]);
						SetLowColor(fColor[bits >> 4]);
						FillPolygon(&p);
					}
				}
			} else {
				// Direct mode
				for (;;)
				{
					uint8_t bits;
					fDataStream.Read(&bits, 1);
					if (bits == 0xFF) {
						align = false;
						break;
					} else if (bits == 0xFE) {
						align = true;
						break;
					} else if (bits == 0xFD) {
						be_app->PostMessage(B_QUIT_REQUESTED);
						break;
					} else {
						BPolygon p;
						BPoint pts[bits & 0xF];
						for (int i = 0; i < (bits & 0xF); i++) {
							BPoint pt;
							uint8_t x,y;
							fDataStream.Read(&x, 1);
							fDataStream.Read(&y, 1);
							pts[i].x = x;
							pts[i].y = y;
						}
						p.AddPoints(pts, bits & 0xF);
						SetHighColor(fColor[bits >> 4]);
						SetViewColor(fColor[bits >> 4]);
						SetLowColor(fColor[bits >> 4]);
						FillPolygon(&p);
					}
				}
			}

			fFrame++;
			if (align) {
				off_t position = fDataStream.Position();
				position &= ~0xFFFF;
				position += 0x10000;
				fDataStream.Seek(position, SEEK_SET);
			}

			if (gBenchmark)
				Invalidate();
		}

		void Pulse()
		{
			Invalidate();
		}

		void KeyDown(const char* bytes, int32 numBytes)
		{
		}

	private:
		// FIXME use a buffered data io for extra speed!
		BBufferIO fDataStream;
		rgb_color fColor[16];
		int fFrame;
};

int main(int argc, char** argv)
{
	BApplication app("application/x-vnd.Shinra.benicc");

	BScreen screen;
	BRect r = screen.Frame();

	if (argc > 1) {
		// Benchmark mode
		gBenchmark = true;
	} else {
		BAlert* alert = new BAlert("Pick a choice",
			"BeNICCC - By PulkoMandy/Shinra\n"
			"Original polygonstream from STNICCC 2000 by Oxygene\n"
			"Music by Laxity", "Demo", "Benchmark", "Die");
		int32 button = alert->Go();

		if (button == 2) exit(0);
		if (button == 1) gBenchmark = true;
	}

	if (gBenchmark) {
		r = BRect(10, 30, 10 + 256 - 2, 30 + 200 - 2);
	} else {
		float wi = r.Width();
		r.right = r.left + r.Height() * 256 / 200;
		r.OffsetBy((wi - r.Width()) / 2, 0);
	}
	BWindow* w = new BWindow(r,
		"BeNICCC", B_TITLED_WINDOW, 0, 0);

	BGroupView* v = new DemoView();
	w->SetLayout(new BGroupLayout(B_VERTICAL));
	w->AddChild(v);
	v->MakeFocus();

	if (!gBenchmark)
		w->SetPulseRate(1000000 / 60);
	w->SetFlags(B_CLOSE_ON_ESCAPE | B_QUIT_ON_WINDOW_CLOSE | B_NOT_RESIZABLE);

	w->Show();

	BFileGameSound s("DESERTD2.MOD", false);
	if (!gBenchmark) {
		if (s.StartPlaying() != B_OK) {
			fprintf(stderr, "Error playing music\n");
		}
	}

	bigtime_t start = system_time();
	app.Run();
	bigtime_t end = system_time();

	if (gBenchmark) {
		BString message;
		message.SetToFormat("Benchmark completed in %f seconds", (end - start) / 1000000.0f);
		BAlert* a = new BAlert("benchmark result", message.String(), "Wow!");
		a->Go();
	}

	s.StopPlaying();

	return 0;
}
