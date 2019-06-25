import Tkinter as tk
import tkMessageBox
import thread
import time
import urllib

from PIL import Image, ImageTk

import imageread
import data.settings as s
import pub as pub



try:

    import setup_data
except ImportError:


    sys.exit(1)


app = None
camera = None
has_quit = False
occupancy = [None for i in range(10)]


class MainApplication(tk.Frame):

    __is_verbose = s.IS_VERBOSE


    __camera = None
    __preview_is_active = False

    __label = ""


    def __init__(self, master = None):

        tk.Frame.__init__(self, master)


        self.grid()

        global camera
        self.__camera = camera
        self.__camera.awb_mode = 'auto'
        self.__camera.exposure_mode = 'auto'

        self.updateText()


        self.bind("<Escape>", self.escapePressHandler)
        self.focus_set()

    def updateText(self):
        num_spaces = 0
        occupied = 0

        global occupancy


        for i in occupancy:
            if i != None: num_spaces += 1
            if i == True: occupied += 1

	
	msg = " ["+str(num_spaces - occupied) +'/' + str(num_spaces)+"]"+" SLOT"
	pub.sendMessage(msg)


    def escapePressHandler(self, event):

        if self.__camera and self.__preview_is_active:
            self.__camera.stop_preview()
            self.__preview_is_active = False

            self.focus_set()

    def clickStartPreview(self):

        if self.__camera and not self.__preview_is_active:

            tkMessageBox.showinfo(
                title = "Show Camera Feed",
                message = "Press the ESCAPE key to exit preview mode"
                )


            self.__camera.start_preview()
            self.__preview_is_active = True


            self.focus_set()

    def clickQuit(self):

        global has_quit

        self.quit()
        self.master.destroy()
        has_quit = True

    def __createWidgets(self):


        self.preview_button = tk.Button(self, text = "Show Camera Feed",
            command = self.clickStartPreview)
        self.preview_button.grid(row = 1, column = 0, 
            sticky = tk.W + tk.E + tk.N + tk.S)

        self.quit_button = tk.Button(self, text = "Quit",
            command = self.clickQuit)
        self.quit_button.grid(row = 1, column = 1, 
            sticky = tk.W + tk.E + tk.N + tk.S)

    def loadImage(self, image_address, canvas, width, height):

        canvas.delete(tk.ALL)

        try:

            if not isinstance(canvas, tk.Canvas): raise TypeError
            if not isinstance(image_address, str): raise TypeError
            if not isinstance(width, int): raise TypeError
            if not isinstance(height, int): raise TypeError

            photo = ImageTk.PhotoImage(Image.open(image_address))
            canvas.create_image((width, height), image = photo)
            canvas.image = photo

            return True

        except TypeError:
            return False
        except:
            return False

    def getIsVerbose():
        return self.__is_verbose



def create_application():
    global app

    root = tk.Tk()
    app = MainApplication(master = root)
    app.master.title("Parking lot")
    app.mainloop()


def run():

    global camera
    global occupancy
    global app

    image_location = "./images/pipark.jpeg" 
    loop_delay = s.PICTURE_DELAY

    space_boxes, control_boxes = __setup_box_data()
    num_spaces = len(space_boxes)
    num_controls = len(control_boxes)


    assert num_spaces > 0
    assert num_controls == 3

    last_status = [None for i in range(10)]
    last_ticks = [3 for i in range(10)]
    while True:
        space_averages = []
        control_averages = []

        camera.capture(image_location)

        try:
            image = imageread.Image.open(image_location)
            pixels = image.load()
        except:
            print "ERROR: Check camera"
            sys.exit(1)

        for space in space_boxes:
            space_x = space[2]
            space_y = space[3]
            space_w = abs(space[4] - space[2])
            space_h = abs(space[5] - space[3])

            space_average = imageread.get_area_average(
                pixels, 
                space_x, 
                space_y, 
                space_w, 
                space_h
                )
            space_averages.append(space_average)


        for control in control_boxes:
            control_x = control[2]
            control_y = control[3]
            control_w = abs(control[4] - control[2])
            control_h = abs(control[5] - control[3])

            control_average = imageread.get_area_average(
                pixels, 
                control_x, 
                control_y, 
                control_w, 
                control_h
                )
            control_averages.append(control_average)


        if s.IS_VERBOSE: print "\n\n"

        for i, space in zip(space_boxes, space_averages):

            num_controls = 0

            print "",

            for control in control_averages:


                if imageread.compare_area(space, control):
                    num_controls += 1
                    print "",
                else:
                    print "",


            is_occupied = False
            if num_controls >= 2: is_occupied = True

            if s.IS_VERBOSE and is_occupied:
                print "Number", i[0], "is filled.\n"
            elif s.IS_VERBOSE and not is_occupied:
                print "Number", i[0], "is empty.\n"


            if last_status[i[0]] != is_occupied:

                if last_ticks[i[0]] < 2:
                    last_ticks[i[0]] += 1
                else:
                    last_status[i[0]] = is_occupied
                    last_ticks[i[0]] = 1

                    num = 1 if is_occupied else 0
                    occupancy = last_status


            else:
                last_ticks[i[0]] = 1

        app.updateText()
        imageread.time.sleep(loop_delay)



def main():

    global has_quit
    global camera

    camera = imageread.setup_camera(is_fullscreen = False)

    try:
        thread.start_new_thread(create_application, ())
        thread.start_new_thread(run, ())
    except:
    	print "ERROR: Failed new thread"

    while not has_quit:
    	pass


def __setup_box_data():


    try:
        box_data = setup_data.boxes
        print "INFO: box_data successfully created."
    except:
        print "ERROR: setup_data.py does not contain the variable 'boxes'."
        sys.exit()

    if not box_data:
        print "ERROR: boxes in setup_data.py is empty!"
        sys.exit()
    else:
        print "INFO: box_data contains data!"

    space_boxes = []
    control_boxes = []

    for data_set in box_data:
        if data_set[1] == 0: space_boxes.append(data_set)
        elif data_set[1] == 1: control_boxes.append(data_set)
        else: print "ERROR: Box-type not set to either 0 or 1."

    print "space boxes:", space_boxes, "\ncontrol boxes:", control_boxes
    return space_boxes, control_boxes

if __name__ == "__main__":
    main()


