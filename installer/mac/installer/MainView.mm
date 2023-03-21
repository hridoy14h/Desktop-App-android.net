#import "MainView.h"
#import <SVGKit.h>
#import <QuartzCore/QuartzCore.h>
#include "helper/installhelper_mac.h"
#import "Logger.h"

@implementation MainView

extern std::string g_path;

-(id)initWithCoder:(NSCoder *)coder
{
    self = [super initWithCoder:coder];
    if (self)
    {
        curBackgroundOpacity_ = 1.0;
        isInstallScreen_ = YES;
        isInstalling_ = NO;
        isLaunching_ = NO;
        imageResources_ = [[ImageResources alloc] init];
        bMouseButtonPressed_ = false;
        [self updateState];
        
        progressView_ = [[ProgressView alloc] initWithFrame:NSMakeRect(100, 200, 150, 70)];
        progressView_.hidden = YES;
        [self addSubview:progressView_];
        
        [imageResources_.backgroundImage setSize:self.bounds.size];
        
        // foreground view for text
        fgView_ = [[ForegroundView alloc] initWithFrame:CGRectMake(0,0,
                                                               imageResources_.backgroundImage.size.width,
                                                               imageResources_.backgroundImage.size.height)];
        [self addSubview:fgView_ positioned:NSWindowBelow relativeTo:nil];
        [fgView_ setImageResources:imageResources_];
        
        // background view for gif
        bgView_ = [[NSImageView alloc] initWithFrame:CGRectMake(0,0,
                                                               imageResources_.backgroundImage.size.width,
                                                               imageResources_.backgroundImage.size.height)];
        bgView_.animates = YES;
        bgView_.imageScaling = NSImageScaleNone;
        bgView_.image = imageResources_.backgroundImage;
        [self addSubview:bgView_ positioned:NSWindowBelow relativeTo:nil];
    }
    return self;
}

- (void)awakeFromNib
{
    [super awakeFromNib];
    
    [_appDelegate.installer setObjectForCallback:@selector(installerCallback:) withObject:self];

    [[Logger sharedLogger] logAndStdOut:@"Setting up buttons"];

    // setup install button
    if (_appDelegate.isLegacy) {
        _installButton.title = @"Legacy Install";
    } else {
        _installButton.title = @"Install";
    }
    _installButton.image = imageResources_.installIcon.NSImage;
    _installButton.imagePosition = NSImageRight;
    
    [_installButton sizeToFit];
    
    CGFloat btnWidth = _installButton.bounds.size.width + 18*2;
    CGFloat btnHeight = 44;
    CGFloat btnX = (self.bounds.size.width - btnWidth) / 2.0f;
    CGFloat btnY = (self.bounds.size.height - btnHeight) / 2.0f;
    [_installButton setFrame:NSMakeRect(btnX, btnY, btnWidth, btnHeight)];
    
    [progressView_ setFrame:NSMakeRect(btnX, btnY, btnWidth, btnHeight)];
    [fgView_ setInstallButtonRect:NSMakeRect(btnX, btnY, btnWidth, btnHeight)];
    
    //setup eula button
    [_eulaButton sizeToFit];
    CGFloat btnEulaWidth = _eulaButton.bounds.size.width;
    CGFloat btnEulaHeight = _eulaButton.bounds.size.height;
    btnX = (self.bounds.size.width - btnEulaWidth) / 2.0f;
    btnY = 16;
    [_eulaButton setFrame:NSMakeRect(btnX, btnY, btnEulaWidth, btnEulaHeight)];

    _settingsButton.image = imageResources_.settingsIcon.NSImage;
    _settingsButton.imagePosition = NSImageOnly;
    [_settingsButton sizeToFit];
    btnWidth = imageResources_.settingsIcon.size.width + 14;
    btnHeight = imageResources_.settingsIcon.size.height + 14;
    btnX = (self.bounds.size.width - btnWidth) / 2.0f;
    btnY = btnEulaHeight + 18;
    [_settingsButton setFrame:NSMakeRect(btnX, btnY, btnWidth, btnHeight)];
    
    if (_appDelegate.isLegacy)
    {
        [_settingsButton removeFromSuperview];
    }
    
    // setup esc button
    _escButton.hidden = YES;
    _escButton.image = imageResources_.checkIcon.NSImage;
    _escButton.imagePosition = NSImageRight;
    
    btnWidth = MAX(_escButton.attributedTitle.size.width + 2 * 2, imageResources_.checkIcon.size.width + 2*2);
    btnHeight = _escButton.attributedTitle.size.height + 2 * 4 + imageResources_.checkIcon.size.height;
    btnX = (self.bounds.size.width - btnWidth) / 2.0f;
    btnY =  10;
    [_escButton setFrame:NSMakeRect(btnX, btnY, btnWidth, btnHeight)];
    
    // setup factory reset toggle
    _factoryResetToggle.hidden = YES;

    btnWidth = imageResources_.toggleBgWhite.size.width;
    btnHeight = imageResources_.toggleBgWhite.size.height;
    btnX = self.bounds.size.width - btnWidth - 16;
    btnY = 100;
    [_factoryResetToggle setFrame:NSMakeRect(btnX, btnY, btnWidth, btnHeight)];
    [_factoryResetToggle setImageResources:imageResources_];

     // factory reset text field
    _factoryResetField.hidden = YES;
    _factoryResetField.stringValue = @"Factory Reset";
    [_factoryResetField setFrame:NSMakeRect(16, btnY, self.bounds.size.width - btnHeight - 16 * 3, btnHeight)];

    [self updateState];

    if (_appDelegate.isUpdateMode) {
        [[Logger sharedLogger] logAndStdOut:[NSString stringWithFormat:@"Application location: %@", [NSString stringWithUTF8String:g_path.c_str()]]];

        // if there is a path set that's not the default, warn the user that we're migrating them to /Applications
        if(g_path.empty() || g_path.rfind("/Applications/Windscribe.app", 0) != 0) {
            NSAlert *alert = [[NSAlert alloc] init];
            [alert addButtonWithTitle:@"Continue"];
            [alert addButtonWithTitle:@"Cancel"];
            [alert setMessageText:@"Windscribe Installer"];
            [alert setInformativeText:@"To improve security, Windscribe will be moved to /Applications."];
            [alert setAlertStyle:NSAlertStyleWarning];

            [alert beginSheetModalForWindow:[self window] completionHandler:^(NSInteger result) {
                if (result == NSAlertFirstButtonReturn) {
                    [self->_installButton performClick:self];
                } else if (result == NSAlertSecondButtonReturn) {
                    // launch old app
                    [self.appDelegate.installer runLauncher];
                    [NSApp terminate:self];
                }
            }];

        } else {
            [_installButton performClick:self];
        }
    }
}

- (void)drawRect:(NSRect)dirtyRect
{
    [super drawRect:dirtyRect];
    
    [[NSGraphicsContext currentContext] saveGraphicsState];
    
    NSColor *fgColor = [NSColor colorWithCalibratedRed: 0.0f green: 0.0f blue: 0.0f alpha: 1.0f];
    [fgColor set];
    NSRectFill([self bounds]);

    
    [[NSGraphicsContext currentContext] restoreGraphicsState];
}

- (void)mouseDown:(NSEvent *)event
{
    startDragPosition_ = CGPointMake(NSEvent.mouseLocation.x, NSEvent.mouseLocation.y);
    startFrameOrigin_ = self.window.frame.origin;
    bMouseButtonPressed_ = true;
}

- (void)mouseDragged:(NSEvent *)event
{
    if (bMouseButtonPressed_)
    {
        //NSLog(@"%f <-> %f", NSEvent.mouseLocation.x, startDragPosition_.x );
        NSPoint pt = CGPointMake(NSEvent.mouseLocation.x - startDragPosition_.x + startFrameOrigin_.x,
                                 NSEvent.mouseLocation.y - startDragPosition_.y +startFrameOrigin_.y);
        
        [self.window setFrameOrigin: pt];
    }
}

- (void)mouseUp:(NSEvent *)event
{
    bMouseButtonPressed_ = false;
}

- (void)onOpacityTimer
{
    if (isIncOpacity_)
    {
        curBackgroundOpacity_ += 0.05f;
        if (curBackgroundOpacity_ >= 1.0f)
        {
            curBackgroundOpacity_ = 1.0f;
            [opacityTimer_ invalidate];
            opacityTimer_ = nil;
        }
    }
    else
    {
        curBackgroundOpacity_ -= 0.05;
        if (curBackgroundOpacity_ <= 0.3f)
        {
            curBackgroundOpacity_ = 0.3f;
            [opacityTimer_ invalidate];
            opacityTimer_ = nil;
        }
    }
    [self updateState];
    [self setNeedsDisplay:YES];
}

- (IBAction)onEulaClick:(id)sender
{
    [[NSWorkspace sharedWorkspace] openURL:[NSURL URLWithString: @"https://windscribe.com/terms/eula"]];
}
- (IBAction)onInstallClick:(id)sender
{
    // check folder already exists
    if ([_appDelegate.installer isFolderAlreadyExist])
    {
        NSAlert *alert = [[NSAlert alloc] init];
        [alert addButtonWithTitle:@"Replace"];
        [alert addButtonWithTitle:@"Stop"];
        [alert setMessageText:@"Windscribe Installer"];
        [alert setInformativeText:@"An item named \"Windscribe\" already exists in this location. Do you want to replace it?"];
        [alert setAlertStyle:NSAlertStyleWarning];
        
        [alert beginSheetModalForWindow:[self window] completionHandler:^(NSInteger result) {
            
            if (result == NSAlertFirstButtonReturn)
                [self startInstall];
        }];
    }
    else
    {
        [self startInstall];
    }
}

- (void)startInstall
{
    _settingsButton.enabled = NO;
    [progressView_ startAnimation];
    progressView_.hidden = NO;
    _installButton.hidden = YES;
    isInstalling_ = TRUE;
    [self updateState];
    [self setNeedsDisplay:YES];
    [_appDelegate.installer start];
}

- (IBAction)onSettingsClick:(id)sender
{
    isIncOpacity_ = NO;
    isInstallScreen_ = NO;
    _installButton.hidden = YES;
    _eulaButton.hidden = YES;
    _escButton.hidden = NO;
    _settingsButton.hidden = YES;
    _factoryResetToggle.hidden = NO;
    _factoryResetField.hidden = NO;
    opacityTimer_ = [NSTimer scheduledTimerWithTimeInterval:0.01 target:self selector:@selector(onOpacityTimer) userInfo:nil repeats:YES];
    [self updateState];

}

- (IBAction)onEscClick:(id)sender
{
    isIncOpacity_ = YES;
    isInstallScreen_ = YES;
    _installButton.hidden = NO;
    _eulaButton.hidden = NO;
    _escButton.hidden = YES;
    _settingsButton.hidden = NO;
    _factoryResetToggle.hidden = YES;
    _factoryResetField.hidden = YES;
    opacityTimer_ = [NSTimer scheduledTimerWithTimeInterval:0.01 target:self selector:@selector(onOpacityTimer) userInfo:nil repeats:YES];
    [self updateState];
}

- (IBAction)onFactoryResetToggleClick:(id)sender
{
    _appDelegate.installer.factoryReset = _factoryResetToggle.checked;
}

- (void) installerCallback: (id)object
{
    BaseInstaller *installer = object;
    if (installer.currentState == STATE_EXTRACTING)
    {
        [progressView_ setProgress: installer.progress];
    }
    else if (installer.currentState == STATE_ERROR)
    {
        NSAlert *alert = [[NSAlert alloc] init];
        NSString *errStr = @"Error during installation.";
        [alert setMessageText:errStr];
        [[Logger sharedLogger] logAndStdOut:errStr];

        [alert setInformativeText: installer.lastError];
        [alert setAlertStyle:NSAlertStyleCritical];

        [alert beginSheetModalForWindow:[self window] completionHandler:^(NSInteger result) {
            [NSApp terminate:self];
        }];
    }
    else if (installer.currentState == STATE_FINISHED)
    {
        static bool isAlreadyFinished = false;
        // for prevent duplicate run
        if (!isAlreadyFinished)
        {
            [progressView_ setProgress: installer.progress];
            isLaunching_ = YES;
            [self updateState];
            [self setNeedsDisplay:YES];
            
            [[Logger sharedLogger] logAndStdOut:[NSString stringWithFormat:@"Finished! (%@)", [installer getInstallPath]]];
            
            isAlreadyFinished = true;

            // we make a delay of 1 ms to update the GUI
            dispatch_after(dispatch_time(DISPATCH_TIME_NOW, 1000000), dispatch_get_main_queue(), ^{
                [progressView_ setProgress: -1];
                [self.appDelegate.installer runLauncher];
            });
        }
    }
    else if (installer.currentState == STATE_LAUNCHED)
    {
        [NSApp terminate:self];
    }
}

-(void) updateState{
    if (!isLaunching_ && !isInstallScreen_) {
        [fgView_ setCaption:@"Install Settings"];
    } else {
        [fgView_ setCaption:@""];
    }
    [fgView_ setIsInstallScreen:isInstallScreen_];
    [fgView_ setNeedsDisplay:YES];
    [bgView_ setAlphaValue:curBackgroundOpacity_];
}


@end
