.PHONY: all clean

PROJECT=falling_time

SRC=animation.c bg.c box.c camera.c c_array.c draw.c game.c gap.c high_score.c init.c input.c main.c particle.c pickup.c player.c sound.c space.c text.c title.c
SRC+=platform/general.c
SRC+=$(addprefix chipmunk/src/,chipmunk.c cpArbiter.c cpArray.c cpBBTree.c cpBody.c cpCollision.c cpConstraint.c cpDampedRotarySpring.c cpDampedSpring.c cpGearJoint.c cpGrooveJoint.c cpHashSet.c cpHastySpace.c cpMarch.c cpPinJoint.c cpPivotJoint.c cpPolyline.c cpPolyShape.c cpRatchetJoint.c cpRotaryLimitJoint.c cpShape.c cpSimpleMotor.c cpSlideJoint.c cpSpace.c cpSpaceComponent.c cpSpaceDebug.c cpSpaceHash.c cpSpaceQuery.c cpSpaceStep.c cpSpatialIndex.c cpSweep1D.c)

CFLAGS=-I. -Ichipmunk/include $(shell pkg-config --cflags --libs sdl SDL_image SDL_mixer SDL_ttf) -lm -DNDEBUG

all: $(PROJECT)

$(PROJECT): $(SRC)
	gcc -o $@ $^ $(CFLAGS)

clean:
	rm -rf $(PROJECT)
