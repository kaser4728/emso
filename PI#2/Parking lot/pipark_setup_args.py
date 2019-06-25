import os
import Tkinter as tk
import tkMessageBox


class Application(tk.Frame):


    def __init__(self, master = None):
        

        tk.Frame.__init__(self, master)
        

        self.grid(padx = 10, pady = 10)
        
        self.__createMenu()  
    


    def __createMenu(self):
        
        PADDING = 10;

      
        try:
            import data.settings as s
            self.defaults = [s.WAKEUP_DELAY,
                     s.PICTURE_DELAY,
                     s.PICTURE_RESOLUTION[0],
                     s.PICTURE_RESOLUTION[1],
                     s.CAMERA_WINDOW_SIZE[2],
                     s.CAMERA_WINDOW_SIZE[3],
                     s.IMAGE_THRESHOLD,
                     s.IS_VERBOSE,
                     s.PARK_ID,
                     s.SERVER_PASS,
                     s.SERVER_URL]
        except:
            
            self.defaults = ['' for i in range(11)]

     
        self.inputs = [['Wakeup Delay', 'int', 'WAKEUP_DELAY'],
                 ['Picture Delay', 'int', 'PICTURE_DELAY'],
                 ['Picture Resolution W', 'int', 'PICTURE_RESOLUTION[0]'],
                 ['Picture Resolution H', 'int', 'PICTURE_RESOLUTION[1]'],
                 ['Camera Window W', 'int', 'CAMERA_WINDOW_SIZE[2]'],
                 ['Camera Window H', 'int', 'CAMERA_WINDOW_SIZE[3]'],
                 ['Image Threshold', 'int', 'IMAGE_THRESHOLD'],
                 ['Is Verbose?', 'check', 'IS_VERBOSE'],
                 ['Park ID', 'int', 'PARK_ID'],
                 ['Server Password', 'text', 'SERVER_PASS'],
                 ['Server URL', 'text', 'SERVER_URL']]

     
        self.options = [[] for i in range(len(self.inputs))]

        for i in range(0, len(self.options)):
            self.options[i] = [0, 0, 0]

            self.options[i][0] = tk.Label(self,
                                          text = self.inputs[i][0] + ' (' + self.inputs[i][1] + ')',
                                          padx = PADDING, justify = tk.LEFT)
            self.options[i][0].grid(row = i, column = 1, sticky = tk.W + tk.E + tk.N + tk.S)

            if self.inputs[i][1] == 'text' or self.inputs[i][1] == 'int':
                self.options[i][2] = tk.StringVar()
                self.options[i][1] = tk.Entry(self, width = 30, textvariable=self.options[i][2])
                self.options[i][2].set(self.defaults[i])

            elif self.inputs[i][1] == 'check':
                self.options[i][2] = tk.IntVar()
                self.options[i][1] = tk.Checkbutton(self, variable=self.options[i][2])
                self.options[i][2].set(1 if self.defaults[i] else 0)
    
            self.options[i][1].grid(row = i, column = 2, sticky = tk.W + tk.E + tk.N + tk.S, pady = 2)

        self.save_button = tk.Button(self, text = "Save Settings", command = self.save, padx = PADDING, pady = PADDING)
        self.save_button.grid(row = len(self.options), column = 1, sticky = tk.W + tk.E + tk.N + tk.S, columnspan = 2, pady = 2)


    def save(self):
        """ For saving the outputs to a file."""
        fail = False

        BREAK = '\r\n'
    
        ostr  = "from uuid import getnode as get_mac" + BREAK
        ostr += "PI_ID = get_mac()" + BREAK
        ostr += "CAMERA_WINDOW_SIZE = [0, 0, 0, 0]" + BREAK
        ostr += "PICTURE_RESOLUTION = [0, 0]" + BREAK
                
        for i in range(len(self.inputs)):
            option = self.inputs[i]
            if option[1] == 'text':
                ostr += option[2] + ' = "' + self.options[i][1].get() + '"' + BREAK
            elif option[1] == 'int':
                try:
                    ostr += option[2] + ' = ' + str(int(self.options[i][1].get())) + BREAK
                except ValueError:
                    print "ERROR: Number is not an int."
                    fail = True
                    break 
            elif option[1] == 'check':
                ostr += option[2] + ' = ' + ("True" if self.options[i][2].get() == 1 else "False") + BREAK

        if not fail:
            
            f1 = open('./data/settings.py', 'w+')

           
            print >>f1, ostr
        
            print 'Settings saved in file /data/settings.py.'
            tkMessageBox.showinfo("Pi Setup", "Settings saved in file.")
        else:
            tkMessageBox.showerror("Pi Setup", "Error in inputs. Did not write to file.")
            
        return not fail

# Main Program
if __name__ == "__main__":
    root = tk.Tk()
    app = Application(master = root)
    app.master.title("PiPark Setup")
    app.mainloop()
