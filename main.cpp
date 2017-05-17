#include <SDL.h>
#include <SDL_opengl.h>

#include <Leap.h>

#include <glm/glm.hpp>

Leap::Frame last_frame;

bool rotate = false;

#define POINTER_ID_INVALID INT32_MAX
int32_t pointer_id = POINTER_ID_INVALID;
glm::vec3 pointer_pos, pointer_dir;

#define TEXTURE_WORLD_Z -10.f
#define TEXTURE_WORLD_W 16.f
#define TEXTURE_WORLD_H 9.f
#define TEXTURE_W 640
#define TEXTURE_H 360
glm::vec3 texture_data[TEXTURE_W * TEXTURE_H];
GLuint texture;

void clear_texture();
void reshape(int, int);

bool handle_events()
{
	SDL_Event ev;
	while(SDL_PollEvent(&ev))
	{
		switch(ev.type)
		{
			case SDL_QUIT:
			return false;

			case SDL_KEYDOWN:
			{
				switch(ev.key.keysym.scancode)
				{
					case SDL_SCANCODE_SPACE:
					rotate = !rotate;
					break;

					case SDL_SCANCODE_C:
					clear_texture();
					break;
				}
			}
			break;

			case SDL_WINDOWEVENT:
			{
				switch(ev.window.event)
				{
					case SDL_WINDOWEVENT_RESIZED:
					reshape(ev.window.data1, ev.window.data2);
					break;
				}
			}
			break;

			default:
			break;
		}
	}

	return true;
}

void handle_leap(const Leap::Controller &controller)
{
	{
		static bool last_connected = false;
		const bool connected = controller.isConnected();

		if(last_connected != connected)
		{
			if(!connected) printf("Leap disconnected\n");
			else printf("Leap connected\n");
		}

		last_connected = connected;

		if(!connected) return;
	}

	const Leap::Frame frame = controller.frame();

	if(pointer_id != POINTER_ID_INVALID)
	{
		const auto finger = frame.finger(pointer_id);
		if(!finger.isValid()) pointer_id = POINTER_ID_INVALID;
	}
	if(pointer_id == POINTER_ID_INVALID)
	{
		const auto hands = frame.hands();
		for(const auto hand : hands)
		{
			const auto finger = hand.fingers()[Leap::Finger::TYPE_INDEX];
			if(!finger.isValid()) continue;

			pointer_id = finger.id();
		}
	}
	if(pointer_id != POINTER_ID_INVALID)
	{
		const auto bone = frame.finger(pointer_id).bone(Leap::Bone::TYPE_DISTAL);
		pointer_pos = glm::vec3(bone.prevJoint().x, bone.prevJoint().y, bone.prevJoint().z) / 100.f;
		pointer_dir = -glm::vec3(bone.direction().x, bone.direction().y, bone.direction().z);
	}

	last_frame = frame;
}

void clear_texture()
{
	for(int y=0; y<TEXTURE_H; ++y) for(int x=0; x<TEXTURE_W; ++x)
	{
		//texture_data[y*TEXTURE_W+x] = (int)(x/128 + y/128) % 2 ? glm::vec3(0.f) : glm::vec3(.3f);
		texture_data[y*TEXTURE_W+x] = glm::vec3(.1f);
	}
}

void init()
{
	clear_texture();

	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, TEXTURE_W, TEXTURE_H, 0, GL_RGB, GL_FLOAT, &texture_data[0][0]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
}

void reshape(int w, int h)
{
	glViewport(0, 0, w, h);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glFrustum(-.8f, .8f, -.8f * h / w, .8f * h / w, 1.f, 100.f);
	glMatrixMode(GL_MODELVIEW);
}

void update()
{
	if(pointer_id != POINTER_ID_INVALID)
	{
		const float t = (TEXTURE_WORLD_Z - pointer_pos.z) / pointer_dir.z;
		if(t > 0.f)
		{
			glm::vec3 tp = pointer_pos + pointer_dir * t;
			glm::vec2 tc = (glm::vec2(tp.x, tp.y) - glm::vec2(-TEXTURE_WORLD_W/2.f, 0.f)) * (glm::vec2(TEXTURE_W, TEXTURE_H) / glm::vec2(TEXTURE_WORLD_W, TEXTURE_WORLD_H));
			int x = (int)tc.x, y = (int)tc.y;
			if(x >= 0 && x < TEXTURE_W && y >= 0 && y < TEXTURE_H)
			{
				texture_data[y*TEXTURE_W+x] = glm::vec3(1.f, 0.f, 0.f);
			}
		}
	}
}

void draw()
{
	glClear(GL_COLOR_BUFFER_BIT);

	glLoadIdentity();
	glTranslatef(0.f, -TEXTURE_WORLD_H/2.f, -20.f);
	if(rotate) glRotatef(SDL_GetTicks()/20.f, 0.f, 1.f, 0.f);

	glBegin(GL_LINES);
	glColor3f(1.f, 0.f, 0.f);
	glVertex3f(0.f, 0.f, 0.f);
	glVertex3f(1.f, 0.f, 0.f);
	glColor3f(0.f, 1.f, 0.f);
	glVertex3f(0.f, 0.f, 0.f);
	glVertex3f(0.f, 1.f, 0.f);
	glColor3f(0.f, 0.f, 1.f);
	glVertex3f(0.f, 0.f, 0.f);
	glVertex3f(0.f, 0.f, 1.f);
	glEnd();

	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, TEXTURE_W, TEXTURE_H, GL_RGB, GL_FLOAT, texture_data);
	glEnable(GL_TEXTURE_2D);
	glBegin(GL_QUADS);
	glColor3f(1.f, 1.f, 1.f);
	glTexCoord2f(0.f, 0.f);
	glVertex3f(-TEXTURE_WORLD_W/2.f, 0.f, TEXTURE_WORLD_Z);
	glTexCoord2f(1.f, 0.f);
	glVertex3f(TEXTURE_WORLD_W/2.f, 0.f, TEXTURE_WORLD_Z);
	glTexCoord2f(1.f, 1.f);
	glVertex3f(TEXTURE_WORLD_W/2.f, TEXTURE_WORLD_H, TEXTURE_WORLD_Z);
	glTexCoord2f(0.f, 1.f);
	glVertex3f(-TEXTURE_WORLD_W/2.f, TEXTURE_WORLD_H, TEXTURE_WORLD_Z);
	glEnd();
	glDisable(GL_TEXTURE_2D);

	if(pointer_id != POINTER_ID_INVALID)
	{
		glm::vec3 p1, p2;
		p1 = pointer_pos;
		p2 = p1 + pointer_dir * 3.f;

		glBegin(GL_LINES);
		glColor3f(1.f, 1.f, 1.f);
		glVertex3fv(&p1[0]);
		glVertex3fv(&p2[0]);
		glVertex3f(p1.x-.1f, p1.y-.1f, p1.z);
		glVertex3f(p1.x+.1f, p1.y+.1f, p1.z);
		glVertex3f(p1.x-.1f, p1.y+.1f, p1.z);
		glVertex3f(p1.x+.1f, p1.y-.1f, p1.z);
		glEnd();
	}

	glBegin(GL_LINES);
	glColor3f(1.f, 0.f, 0.f);
	for(const auto hand : last_frame.hands())
	{
		for(const auto finger : hand.fingers())
		{
			for(int i=0; i<4; ++i)
			{
				const auto bone = finger.bone(static_cast<Leap::Bone::Type>(i));

				glm::vec3 p1, p2;
				p1 = glm::vec3(bone.prevJoint().x, bone.prevJoint().y, bone.prevJoint().z) / 100.f;
				p2 = glm::vec3(bone.nextJoint().x, bone.nextJoint().y, bone.nextJoint().z) / 100.f;

				glVertex3fv(&p1[0]);
				glVertex3fv(&p2[0]);
			}
		}
	}
	glEnd();
}

int main(int argc, char *argv[])
{
	SDL_Init(SDL_INIT_VIDEO);

	SDL_Window *window = SDL_CreateWindow("LeapTest1", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
	SDL_GLContext context = SDL_GL_CreateContext(window);

	SDL_GL_SetSwapInterval(1);

	Leap::Controller controller;

	init();
	reshape(1280, 720);

	for(;;)
	{
		if(!handle_events()) break;
		handle_leap(controller);

		update();
		draw();

		SDL_GL_SwapWindow(window);
	}

	SDL_GL_DeleteContext(context);
	SDL_DestroyWindow(window);

	SDL_Quit();

	return 0;
}
