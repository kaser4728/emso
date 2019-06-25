import Tkinter as tk


class ParkingSpace:
    # replacement for setup_classes.Area
    
    # instance attributes
    __id = -1
    __type = 0  # type '0' is parking space, type '1' is CP
    __start_point = []
    __end_point = []
    __label = ""  # __rectangle label
    __rectangle = None  # the drawn rectangle
    canvas = None
    __label = None
    
    def __init__(self, i, canvas = None):
        # set space id number
        self.__id = i
        self.canvas = canvas
    
    
    def clear(self):
    	"""Clear the coordinates of the space. """
        self.__start_point = []
        self.__end_point = []
        
        return self
    
    
    def setStartPoint(self, x, y):

        # guard against invalid arguments
        if not isinstance(x, int) and not isinstance(y, int):
            print "ERROR: Cannot set start point: x & y must be integers."
        
        # set the start point and return the space   
        self.__start_point = [x, y]
        return self
    
    
    def setEndPoint(self, x, y):
 
        # guard against invalid arguments
        if not isinstance(x, int) and not isinstance(y, int):
            print "ERROR: Cannot set end point: x & y must be integers."
        
        # set the end point and return the space   
        self.__end_point = [x, y]
        return self
    
    
    def updatePoints(self, x, y):
    
        if self.__start_point == [] or self.__end_point != []:
            self.clear()
            self.deleteRectangle(self.canvas)
            self.setStartPoint(x, y)
        else:
            self.setEndPoint(x, y)
            self.drawRectangle(self.canvas)
    
    
    def drawRectangle(self, canvas):
    	
        # guard against illegal data types
        if not isinstance(canvas, tk.Canvas): return
        
        # if either start or end point doesn't exist; a rectangle cannot be
        # drawn, so return
        if self.__start_point == [] or self.__end_point == []: return
        
        # set rectangle colour
        fill_colour = "#CC0000"
        outline_colour = "#990000"
        
        # draw the rectangle
        self.__rectangle = canvas.create_rectangle(
            self.__start_point[0], self.__start_point[1],
            self.__end_point[0], self.__end_point[1],
            fill = fill_colour,
            outline = outline_colour, 
            width = 0,
            stipple = "gray50"
            )
        
        self.__label = canvas.create_text(self.getOrigins(), text = (str(self.__id) + "(space)"))
        return self
        
    
    def deleteRectangle(self, canvas):
    	
        canvas.delete(self.__rectangle)
        canvas.delete(self.__label)
        return self

    def getOrigins(self):
    	
        result = []
        
        if self.__start_point[0] < self.__end_point[0]: result.append(self.__start_point[0])
        else: result.append(self.__end_point[0])
        
        if self.__start_point[1] < self.__end_point[1]: result.append(self.__start_point[1])
        else: result.append(self.__end_point[1])
        
        return result
        
    def getOutput(self):
   
        
        if self.__start_point != [] and self.__end_point != []:
            space = (
                self.__id, 
                self.__type,
                self.__start_point[0],
                self.__start_point[1],
                self.__end_point[0],
                self.__end_point[1]
                )

            return space
        else:
            return None


class ControlPoint:
    # replacement for setup_clases.Area
    
    # instance attributes
    __id = -1
    __type = 1  # type '1' is parking space, type '1' is CP
    __start_point = []
    __end_point = []
    __label = ""  # __rectangle label
    __rectangle = None  # the drawn rectangle
    __label = None
    canvas = None
    
    def __init__(self, i, canvas):
        self.__id = i
        self.canvas = canvas
        return
    
    def clear(self):
    	"""Clear the coordinates of the space. """
        self.__start_point = []
        self.__end_point = []
        
        return self
    
    def setStartPoint(self, x, y):

        if not isinstance(x, int) and not isinstance(y, int):
            print "ERROR: Cannot set start point: x & y must be integers."
        
        # set the start point and return the space   
        self.__start_point = [x, y]
        return self
    
    
    def setEndPoint(self, x, y):

        # guard against invalid arguments
        if not isinstance(x, int) and not isinstance(y, int):
            print "ERROR: Cannot set end point: x & y must be integers."
        
        # set the end point and return the space   
        self.__end_point = [x, y]
        return self
    
    

        x1 = x - 25
        y1 = y - 25
        x2 = x + 25
        y2 = y + 25
        
        self.clear()
        self.deleteRectangle(self.canvas)
        self.setStartPoint(x1, y1)
        self.setEndPoint(x2, y2)
        self.drawRectangle(self.canvas)
        
    def drawRectangle(self, canvas):
    	
        # guard against illegal data types
        if not isinstance(canvas, tk.Canvas): return
        
        # if either start or end point doesn't exist; a rectangle cannot be
        # drawn, so return
        if self.__start_point == [] or self.__end_point == []: return
        
        # set rectangle colour
        fill_colour = "#0066CC"
        outline_colour = "#003399"
        
        # draw the rectangle
        self.__rectangle = canvas.create_rectangle(
            self.__start_point[0], self.__start_point[1],
            self.__end_point[0], self.__end_point[1],
            fill = fill_colour,
            outline = outline_colour, 
            width = 0,
            stipple = "gray50"
            )
        
        display_id = str(self.__id + 1)
        self.__label = canvas.create_text([self.__start_point[0], self.__start_point[1]], text = display_id + "(control)")
        return self
        
    
    def deleteRectangle(self, canvas):
   
        canvas.delete(self.__rectangle)
        canvas.delete(self.__label)
        return self

    def getOutput(self):
    	
        
        if self.__start_point != [] and self.__end_point != []:
            cp = (
                self.__id, 
                self.__type,
                self.__start_point[0],
                self.__start_point[1],
                self.__end_point[0],
                self.__end_point[1]
                )

            return cp
        else:
            return None
        



class Boxes:
    boxes = []
    
    current_box = 1
    __type = 0
    
    MAX_SPACES = 10
    MAX_CPS = 3
    
    def __init__(self, canvas, type = 0):
        if type == 0:
            self.__type = 0
            self.boxes = [ParkingSpace(i, canvas) for i in range(self.MAX_SPACES)]
        elif type == 1:
            self.__type = 1
            self.current_box = 0
            self.boxes = [ControlPoint(j, canvas) for j in range(self.MAX_CPS)]
        else:
            print "ERROR: setup_classes.Boxes requires type 0 or 1."
        return
    


    def getCurrentBox(self):
        return self.current_box

    def get(self, id):
        return self.boxes[id]

    def length(self):
        return len(self.boxes)

    def setCurrentBox(self, i):
        self.current_box = i
    
    def clearAll(self, canvas):

        if self.__type == 0: self.setCurrentBox(1)
        elif self.__type == 1: self.setCurrentBox(0)

        for box in self.boxes:
            box.clear()
            box.deleteRectangle(canvas)