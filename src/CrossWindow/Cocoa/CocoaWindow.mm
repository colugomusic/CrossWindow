#include "Window.h"
#import <Cocoa/Cocoa.h>
#import <QuartzCore/CAMetalLayer.h>
#import <QuartzCore/CAOpenGLLayer.h>

@interface XWinWindow : NSWindow
{
}
@end

@implementation XWinWindow

@end

@interface XWinView : NSView
- (BOOL)	acceptsFirstResponder;
- (BOOL)	isOpaque;

@end

@implementation XWinView

- (void)_updateContentScale
{
	NSApplication* nsApp = (NSApplication*)xwin::getXWinState().application;
    NSWindow *mainWindow = [NSApp mainWindow];
    NSWindow *layerWindow = [self window];
    if (mainWindow || layerWindow) {
        CGFloat scale = [(layerWindow != nil) ? layerWindow : mainWindow backingScaleFactor];
        CALayer *layer = self.layer;
        if ([layer respondsToSelector:@selector(contentsScale)]) {
            [self.layer setContentsScale:scale];
        }
    }
}

- (void)scaleDidChange:(NSNotification *)n
{
    [self _updateContentScale];
}

- (void)viewDidMoveToWindow
{
    // Retina Display support
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(scaleDidChange:)
                                                 name:@"NSWindowDidChangeBackingPropertiesNotification"
                                               object:[self window]];

    // immediately update scale after the view has been added to a window
    [self _updateContentScale];
}
- (void)removeFromSuperview
{
    [super removeFromSuperview];
    [[NSNotificationCenter defaultCenter] removeObserver:self name:@"NSWindowDidChangeBackingPropertiesNotification" object:[self window]];
}
- (BOOL)acceptsFirstResponder
{
	return YES;
}

- (BOOL)isOpaque
{
	return YES;
}

@end

namespace xwin {	

Window::Window() {
	window =
	view =
	layer = nullptr;
}

Window::~Window() {
	destroy();
}

void Window::destroy() {
	if( window != nullptr) {
		[(XWinWindow*)window release];
		[(XWinView*)view release];
		[(CALayer*)layer release];
		
		window = nullptr;
		view = nullptr;
		layer = nullptr;
	}
}

void Window::set_position(unsigned x, unsigned y) {
	NSPoint point = NSMakePoint(x, y);
	point  = [((XWinWindow*)window) convertPointToScreen:point];
	[((XWinWindow*)window) setFrameOrigin: point];
}

void Window::set_size(unsigned width, unsigned height) {
	NSRect rect = NSMakeRect(0, 0, width, height);
	[((XWinWindow*)window) setContentSize:rect.size];
	[((XWinWindow*)window) setFrame:rect display:YES];
	[((XWinView*)view) setFrame:rect];
	[((XWinView*)view) setNeedsDisplay:YES];
}

void* Window::get_native_handle() {}
	return view;
}

bool Window::create(const WindowDesc& desc, EventQueue& eventQueue, void* parentView)
{
	NSApplication* nsApp = (NSApplication*)getXWinState().application;
	
	NSRect rect = NSMakeRect(desc.x, desc.y, desc.width, desc.height);
	NSWindowStyleMask styleMask = NSWindowStyleMaskTitled;
	if (desc.closable)
	{
		styleMask |= NSWindowStyleMaskClosable;
	}
	if (desc.resizable)
	{
		styleMask |= NSWindowStyleMaskResizable;
	}
	if (desc.minimizable)
	{
		styleMask |= NSWindowStyleMaskMiniaturizable;
	}
	if (!desc.frame)
	{
		styleMask |= NSWindowStyleMaskFullSizeContentView;
	}
	
	// Setup NSWindow
	window = [[XWinWindow alloc]
			  initWithContentRect: rect
			  styleMask: styleMask
			  backing: NSBackingStoreBuffered
			  defer: NO];
	
	mTitle = [NSString stringWithCString:desc.title.c_str()
								encoding:[NSString defaultCStringEncoding]];
	XWinWindow* w = ((XWinWindow*)window);
	if(!desc.title.empty())
	{ [w setTitle: (NSString*)mTitle]; }
	if(desc.centered)
	{ [w center]; }
	else
	{
		NSPoint point = NSMakePoint(desc.x, desc.y);
		point  = [w convertPointToScreen:point];
		[w setFrameOrigin: point];
	}
	
	[w setHasShadow:desc.hasShadow];
	[w setTitlebarAppearsTransparent:!desc.frame];

	if (parentView) {
		NSView* parentView = (NSView*)(parentView);
		NSWindow* parentWindow = [((NSView*)(parentView)) window];
		[w setLevel:[parentWindow level] + 1];
	}
	else {
		[w setLevel:NSNormalWindowLevel]];
	}

	// Setup NSView
	rect = [w backingAlignedRect:rect options:NSAlignAllEdgesOutward];
	view = [[XWinView alloc] initWithFrame:rect];
	XWinView* v = (XWinView*)view;
	[v setHidden:NO];
	[v setNeedsDisplay:YES];
	[v setWantsLayer:YES];

	[w setContentView:(XWinView*)view];
	[w makeKeyAndOrderFront:nsApp];
	
	eventQueue.update();
	
	mDesc = desc;
	
	return true;
}

void Window::setLayer(LayerType type)
{
	if(type == LayerType::Metal)
	{
		XWinView* v = (XWinView*)view;
		[v setWantsLayer:YES];
		
		layer = [[CAMetalLayer alloc] init];
		CAMetalLayer* l = (CAMetalLayer*)layer;
		[v setLayer:l];
		XWinWindow* w = (XWinWindow*)window;
	}
	else if(type == LayerType::OpenGL)
	{
		
		XWinView* v = (XWinView*)view;
		[v setWantsLayer:YES];
		layer = [[CAOpenGLLayer alloc] init];
		[(XWinView*)view setLayer:(CAOpenGLLayer*)layer];
		CAOpenGLLayer* l = ((CAOpenGLLayer*)layer);
		l.asynchronous = false;
		[l setNeedsDisplay];
		XWinWindow* w = (XWinWindow*)window;
	}
}


void Window::setMousePosition(unsigned x, unsigned y)
{
	CGPoint pos = CGPointMake(x, y);
	CGWarpMouseCursorPosition(pos);
}

UVec2 Window::getCurrentDisplaySize() {
	UVec2 size;
	NSRect screenRect = [[NSScreen mainScreen] frame];
	size.x = screenRect.size.width;
	size.y = screenRect.size.height;
	return size;
}

} // namespace xwin
