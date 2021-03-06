#pragma once
#include <vector>
#include <jansson.h>
#include "widgets.hpp"
#include "ui.hpp"


static const float SVG_DPI = 75.0;
static const float MM_PER_IN = 25.4;

#define CHECKMARK_STRING "✔"
#define CHECKMARK(_cond) ((_cond) ? CHECKMARK_STRING : "")


namespace rack {

inline float in2px(float inches) {
	return inches * SVG_DPI;
}

inline Vec in2px(Vec inches) {
	return inches.mult(SVG_DPI);
}

inline float mm2px(float millimeters) {
	return millimeters * (SVG_DPI / MM_PER_IN);
}

inline Vec mm2px(Vec millimeters) {
	return millimeters.mult(SVG_DPI / MM_PER_IN);
}


struct Model;
struct Module;
struct Wire;

struct RackWidget;
struct ParamWidget;
struct Port;
struct SVGPanel;

////////////////////
// module
////////////////////

// A 1HPx3U module should be 15x380 pixels. Thus the width of a module should be a factor of 15.
static const float RACK_GRID_WIDTH = 15;
static const float RACK_GRID_HEIGHT = 380;
static const Vec RACK_GRID_SIZE = Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT);


struct ModuleWidget : OpaqueWidget {
	Model *model = NULL;
	/** Owns the module pointer */
	Module *module = NULL;

	SVGPanel *panel = NULL;
	std::vector<Port*> inputs;
	std::vector<Port*> outputs;
	std::vector<ParamWidget*> params;

	ModuleWidget(Module *module);
	~ModuleWidget();
	/** Convenience functions for adding special widgets (calls addChild()) */
	void addInput(Port *input);
	void addOutput(Port *output);
	void addParam(ParamWidget *param);
	void setPanel(std::shared_ptr<SVG> svg);

	virtual json_t *toJson();
	virtual void fromJson(json_t *rootJ);

	virtual void create();
	virtual void _delete();
	/** Disconnects cables from all ports
	Called when the user clicks Disconnect Cables in the context menu.
	*/
	virtual void disconnect();
	/** Resets the parameters of the module and calls the Module's randomize().
	Called when the user clicks Initialize in the context menu.
	*/
	virtual void reset();
	/** Deprecated */
	virtual void initialize() final {}
	/** Randomizes the parameters of the module and calls the Module's randomize().
	Called when the user clicks Randomize in the context menu.
	*/
	virtual void randomize();
	/** Do not subclass this to add context menu entries. Use appendContextMenu() instead */
	virtual Menu *createContextMenu();
	/** Override to add context menu entries to your subclass.
	It is recommended to add a blank MenuEntry first for spacing.
	*/
	virtual void appendContextMenu(Menu *menu) {}

	void draw(NVGcontext *vg) override;
	void drawShadow(NVGcontext *vg);

	Vec dragPos;
	void onMouseDown(EventMouseDown &e) override;
	void onMouseMove(EventMouseMove &e) override;
	void onHoverKey(EventHoverKey &e) override;
	void onDragStart(EventDragStart &e) override;
	void onDragEnd(EventDragEnd &e) override;
	void onDragMove(EventDragMove &e) override;
};

struct WireWidget : OpaqueWidget {
	Port *outputPort = NULL;
	Port *inputPort = NULL;
	Port *hoveredOutputPort = NULL;
	Port *hoveredInputPort = NULL;
	Wire *wire = NULL;
	NVGcolor color;

	WireWidget();
	~WireWidget();
	/** Synchronizes the plugged state of the widget to the owned wire */
	void updateWire();
	Vec getOutputPos();
	Vec getInputPos();
	json_t *toJson();
	void fromJson(json_t *rootJ);
	void draw(NVGcontext *vg) override;
	void drawPlugs(NVGcontext *vg);
};

struct WireContainer : TransparentWidget {
	WireWidget *activeWire = NULL;
	/** Takes ownership of `w` and adds it as a child if it isn't already */
	void setActiveWire(WireWidget *w);
	/** "Drops" the wire onto the port, making an engine connection if successful */
	void commitActiveWire();
	void removeTopWire(Port *port);
	void removeAllWires(Port *port);
	/** Returns the most recently added wire connected to the given Port, i.e. the top of the stack */
	WireWidget *getTopWire(Port *port);
	void draw(NVGcontext *vg) override;
};

struct RackWidget : OpaqueWidget {
	FramebufferWidget *rails;
	// Only put ModuleWidgets in here
	Widget *moduleContainer;
	// Only put WireWidgets in here
	WireContainer *wireContainer;
	std::string lastPath;
	Vec lastMousePos;

	RackWidget();
	~RackWidget();

	/** Completely clear the rack's modules and wires */
	void clear();
	/** Clears the rack and loads the template patch */
	void reset();
	void openDialog();
	void saveDialog();
	void saveAsDialog();
	void savePatch(std::string filename);
	void loadPatch(std::string filename);
	json_t *toJson();
	void fromJson(json_t *rootJ);

	void addModule(ModuleWidget *m);
	/** Removes the module and transfers ownership to the caller */
	void deleteModule(ModuleWidget *m);
	void cloneModule(ModuleWidget *m);
	/** Sets a module's box if non-colliding. Returns true if set */
	bool requestModuleBox(ModuleWidget *m, Rect box);
	/** Moves a module to the closest non-colliding position */
	bool requestModuleBoxNearest(ModuleWidget *m, Rect box);
	void step() override;
	void draw(NVGcontext *vg) override;

	void onMouseMove(EventMouseMove &e) override;
	void onMouseDown(EventMouseDown &e) override;
	void onZoom(EventZoom &e) override;
};

struct RackRail : TransparentWidget {
	void draw(NVGcontext *vg) override;
};

struct AddModuleWindow : Window {
	Vec modulePos;

	AddModuleWindow();
	void step() override;
};

struct Panel : TransparentWidget {
	NVGcolor backgroundColor;
	std::shared_ptr<Image> backgroundImage;
	void draw(NVGcontext *vg) override;
};

struct SVGPanel : FramebufferWidget {
	void step() override;
	void setBackground(std::shared_ptr<SVG> svg);
};

////////////////////
// params
////////////////////

struct CircularShadow : TransparentWidget {
	float blur = 0.0;
	void draw(NVGcontext *vg) override;
};

struct ParamWidget : OpaqueWidget, QuantityWidget {
	Module *module = NULL;
	int paramId;
	/** Used to momentarily disable value randomization
	To permanently disable or change randomization behavior, override the randomize() method instead of changing this.
	*/
	bool randomizable = true;
	/** Apply per-sample smoothing in the engine */
	bool smooth = false;

	json_t *toJson();
	void fromJson(json_t *rootJ);
	virtual void reset();
	virtual void randomize();
	void onMouseDown(EventMouseDown &e) override;
	void onChange(EventChange &e) override;

	template <typename T = ParamWidget>
	static T *create(Vec pos, Module *module, int paramId, float minValue, float maxValue, float defaultValue) {
		T *o = Widget::create<T>(pos);
		o->module = module;
		o->paramId = paramId;
		o->setLimits(minValue, maxValue);
		o->setDefaultValue(defaultValue);
		return o;
	}
};

/** Implements vertical dragging behavior for ParamWidgets */
struct Knob : ParamWidget {
	/** Snap to nearest integer while dragging */
	bool snap = false;
	/** Multiplier for mouse movement to adjust knob value */
	float speed = 1.0;
	float dragValue;
	Knob();
	void onDragStart(EventDragStart &e) override;
	void onDragMove(EventDragMove &e) override;
	void onDragEnd(EventDragEnd &e) override;
};

struct SpriteKnob : virtual Knob, SpriteWidget {
	int minIndex, maxIndex, spriteCount;
	void step() override;
};

/** A knob which rotates an SVG and caches it in a framebuffer */
struct SVGKnob : virtual Knob, FramebufferWidget {
	/** Angles in radians */
	float minAngle, maxAngle;
	/** Not owned */
	TransformWidget *tw;
	SVGWidget *sw;

	SVGKnob();
	void setSVG(std::shared_ptr<SVG> svg);
	void step() override;
	void onChange(EventChange &e) override;
};

struct SVGFader : Knob, FramebufferWidget {
	/** Intermediate positions will be interpolated between these positions */
	Vec minHandlePos, maxHandlePos;
	/** Not owned */
	SVGWidget *background;
	SVGWidget *handle;

	SVGFader();
	void step() override;
	void onChange(EventChange &e) override;
};

struct Switch : ParamWidget {
};

struct SVGSwitch : virtual Switch, FramebufferWidget {
	std::vector<std::shared_ptr<SVG>> frames;
	/** Not owned */
	SVGWidget *sw;

	SVGSwitch();
	/** Adds an SVG file to represent the next switch position */
	void addFrame(std::shared_ptr<SVG> svg);
	void onChange(EventChange &e) override;
};

/** A switch that cycles through each mechanical position */
struct ToggleSwitch : virtual Switch {
	void onDragStart(EventDragStart &e) override {
		// Cycle through values
		// e.g. a range of [0.0, 3.0] would have modes 0, 1, 2, and 3.
		if (value >= maxValue)
			setValue(minValue);
		else
			setValue(value + 1.0);
	}
};

/** A switch that is turned on when held */
struct MomentarySwitch : virtual Switch {
	/** Don't randomize state */
	void randomize() override {}
	void onDragStart(EventDragStart &e) override {
		setValue(maxValue);
		EventAction eAction;
		onAction(eAction);
	}
	void onDragEnd(EventDragEnd &e) override {
		setValue(minValue);
	}
};

////////////////////
// IO widgets
////////////////////

struct LedDisplay : Widget {
	void draw(NVGcontext *vg) override;
};

struct LedDisplaySeparator : TransparentWidget {
	LedDisplaySeparator();
	void draw(NVGcontext *vg) override;
};

struct LedDisplayChoice : TransparentWidget {
	std::string text;
	std::shared_ptr<Font> font;
	Vec textOffset;
	NVGcolor color;
	LedDisplayChoice();
	void draw(NVGcontext *vg) override;
	void onMouseDown(EventMouseDown &e) override;
};

struct LedDisplayTextField : TextField {
	std::shared_ptr<Font> font;
	Vec textOffset;
	NVGcolor color;
	LedDisplayTextField();
	void draw(NVGcontext *vg) override;
	int getTextPosition(Vec mousePos) override;
};


struct AudioIO;
struct MidiIO;

struct AudioWidget : LedDisplay {
	/** Not owned */
	AudioIO *audioIO = NULL;
	LedDisplayChoice *driverChoice;
	LedDisplaySeparator *driverSeparator;
	LedDisplayChoice *deviceChoice;
	LedDisplaySeparator *deviceSeparator;
	LedDisplayChoice *sampleRateChoice;
	LedDisplaySeparator *sampleRateSeparator;
	LedDisplayChoice *bufferSizeChoice;
	AudioWidget();
	void step() override;
};

struct MidiWidget : LedDisplay {
	/** Not owned */
	MidiIO *midiIO = NULL;
	LedDisplayChoice *driverChoice;
	LedDisplaySeparator *driverSeparator;
	LedDisplayChoice *deviceChoice;
	LedDisplaySeparator *deviceSeparator;
	LedDisplayChoice *channelChoice;
	MidiWidget();
	void step() override;
};

////////////////////
// lights
////////////////////

struct LightWidget : TransparentWidget {
	NVGcolor borderColor = nvgRGBA(0, 0, 0, 0);
	NVGcolor color = nvgRGBA(0, 0, 0, 0);
	void draw(NVGcontext *vg) override;
	virtual void drawLight(NVGcontext *vg);
	virtual void drawHalo(NVGcontext *vg);
};

/** Mixes a list of colors based on a list of brightness values */
struct MultiLightWidget : LightWidget {
	/** Color of the "off" state */
	NVGcolor bgColor = nvgRGBA(0, 0, 0, 0);
	/** Colors of each value state */
	std::vector<NVGcolor> baseColors;
	void addBaseColor(NVGcolor baseColor);
	/** Sets the color to a linear combination of the baseColors with the given weights */
	void setValues(const std::vector<float> &values);
};

/** A MultiLightWidget that points to a module's Light or a range of lights
Will access firstLightId, firstLightId + 1, etc. for each added color
*/
struct ModuleLightWidget : MultiLightWidget {
	Module *module = NULL;
	int firstLightId;
	void step() override;

	template <typename T = ModuleLightWidget>
	static T *create(Vec pos, Module *module, int firstLightId) {
		T *o = Widget::create<T>(pos);
		o->module = module;
		o->firstLightId = firstLightId;
		return o;
	}
};

////////////////////
// ports
////////////////////

struct Port : OpaqueWidget {
	enum PortType {
		INPUT,
		OUTPUT
	};

	Module *module = NULL;
	PortType type = INPUT;
	int portId;
	MultiLightWidget *plugLight;

	Port();
	~Port();
	void step() override;
	void draw(NVGcontext *vg) override;
	void onMouseDown(EventMouseDown &e) override;
	void onDragStart(EventDragStart &e) override;
	void onDragEnd(EventDragEnd &e) override;
	void onDragDrop(EventDragDrop &e) override;
	void onDragEnter(EventDragEnter &e) override;
	void onDragLeave(EventDragEnter &e) override;

	template <typename T = Port>
	static T *create(Vec pos, PortType type, Module *module, int portId) {
		T *o = Widget::create<T>(pos);
		o->type = type;
		o->module = module;
		o->portId = portId;
		return o;
	}
};

struct SVGPort : Port, FramebufferWidget {
	SVGWidget *background;

	SVGPort();
	void draw(NVGcontext *vg) override;
};

/** If you don't add these to your ModuleWidget, they will fall out of the rack... */
struct SVGScrew : FramebufferWidget {
	SVGWidget *sw;

	SVGScrew();
};

////////////////////
// scene
////////////////////

struct Toolbar : OpaqueWidget {
	Slider *wireOpacitySlider;
	Slider *wireTensionSlider;
	Slider *zoomSlider;
	RadioButton *cpuUsageButton;

	Toolbar();
	void draw(NVGcontext *vg) override;
};

struct PluginManagerWidget : Widget {
	Widget *loginWidget;
	Widget *manageWidget;
	Widget *downloadWidget;
	PluginManagerWidget();
	void step() override;
};

struct RackScrollWidget : ScrollWidget {
	void step() override;
};

struct RackScene : Scene {
	ScrollWidget *scrollWidget;
	ZoomWidget *zoomWidget;

	RackScene();
	void step() override;
	void draw(NVGcontext *vg) override;
	void onHoverKey(EventHoverKey &e) override;
	void onPathDrop(EventPathDrop &e) override;
};

////////////////////
// globals
////////////////////

extern std::string gApplicationName;
extern std::string gApplicationVersion;
extern std::string gApiHost;

// Easy access to "singleton" widgets
extern RackScene *gRackScene;
extern RackWidget *gRackWidget;
extern Toolbar *gToolbar;

void sceneInit();
void sceneDestroy();

json_t *colorToJson(NVGcolor color);
NVGcolor jsonToColor(json_t *colorJ);


} // namespace rack
