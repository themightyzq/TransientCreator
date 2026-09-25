// transient_ui_snapshot: render the real plugin editor headlessly to a PNG.
//
//   transient_ui_snapshot <out.png> [scale] [width height]   (scale defaults to 2.0; width/height
//                                                              default to the editor's own default
//                                                              size, 700x550 -- pass 600 520 to
//                                                              check the minimum resize floor)
//
// UI regression gate for the ZQ SFX house UI style (zqsfx_ui module). Mirrors the sibling
// products' own ui_snapshot tools: links against Transient Creator's own shared-code CMake
// target (the "pamplejuce pattern") instead of recompiling PluginProcessor.cpp/PluginEditor.cpp
// a second time -- see CMakeLists.txt for the target wiring.
//
// Determinism: nothing here ever pumps JUCE's message loop (no runDispatchLoop), so
// TransientControls's 15 Hz juce::Timer and EnvelopeVisualizer's 30 Hz juce::Timer -- either of
// which could make the render depend on wall-clock timing -- never actually fire; JUCE dispatches
// timer callbacks through the message queue, not directly from a background timer thread. The
// snapshot is taken immediately after construction/resize, before any timer tick, which is what
// makes two successive renders of unchanged code byte-identical.

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <iostream>

int main (int argc, char** argv)
{
    if (argc < 2)
    {
        std::cerr << "usage: transient_ui_snapshot <out.png> [scale] [width height]\n";
        return 2;
    }

    juce::ScopedJuceInitialiser_GUI gui;
    const juce::File out = juce::File::getCurrentWorkingDirectory().getChildFile (juce::String (argv[1]));
    const float scale = argc > 2 ? juce::String (argv[2]).getFloatValue() : 2.0f;

    // Processor declared before editor: C++ destroys locals in reverse declaration order, so the
    // editor is always torn down before the processor it references (spec requirement: "Delete
    // the editor before the processor").
    TransientCreatorProcessor processor;
    std::unique_ptr<juce::AudioProcessorEditor> editor (processor.createEditor());
    if (editor == nullptr)
    {
        std::cerr << "createEditor returned null\n";
        return 1;
    }

    if (argc > 4)
    {
        const int w = juce::String (argv[3]).getIntValue();
        const int h = juce::String (argv[4]).getIntValue();
        // Component::setSize() does not consult a ComponentBoundsConstrainer (the editor's fixed
        // 700:550 aspect ratio, set via getConstrainer()->setFixedAspectRatio in the ctor, is only
        // enforced by interactive drag-resizing), so this lands at exactly w x h regardless of
        // aspect ratio -- e.g. the 600x520 minimum-size check below.
        editor->setSize (w, h);
    }

    const auto image = editor->createComponentSnapshot (editor->getLocalBounds(), true, scale);

    out.getParentDirectory().createDirectory();
    out.deleteFile();
    juce::FileOutputStream stream (out);
    juce::PNGImageFormat png;
    if (! stream.openedOk() || ! png.writeImageToStream (image, stream))
    {
        std::cerr << "could not write " << out.getFullPathName() << "\n";
        return 1;
    }

    std::cout << out.getFullPathName() << "  " << image.getWidth() << "x" << image.getHeight() << "\n";
    return 0;
}
