
import Tkinter as tk



class ToggleButton(tk.Button):
    

    __is_active = False
    
 
    ACTIVE_BG = "royalblue"
    ACTIVE_FG = "white"
    ACTIVE_HL = "cornflowerblue"
    NORMAL_BG = "gainsboro"
    NORMAL_FG = "black"
    NORMAL_HL = "#EDEDED"  # this colour doesn't have a name, lets call him bob
    

    def __init__(self, master = None):
     
        
        # run super constructor method
        tk.Button.__init__(self, master)

    def toggle(self):
        
        if self.__is_active:
            self.setOff()
        else:
            self.setOn()
    

    def setOff(self):
        """Set the button to 'normal' (off, untoggled) state. """
        self.__is_active = False
        self.config(
            background = self.NORMAL_BG,
            foreground = self.NORMAL_FG,
            activebackground = self.NORMAL_HL,
            activeforeground = self.NORMAL_FG
        )
        
    def setOn(self):
        """Set the button to 'active' (on, toggled) state. """
        self.__is_active = True
        self.config(
            background = self.ACTIVE_BG,
            foreground = self.ACTIVE_FG,
            activebackground = self.ACTIVE_HL,
            activeforeground = self.ACTIVE_FG
        )
        
     
    def getIsActive(self):
        """Return whether the button is active (True) or not (False). """
        return self.__is_active