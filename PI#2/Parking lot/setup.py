
import sys

# PiPark
import imageread
import main
import setup_selectarea

# Pythonware, Image library
from PIL import Image


# -----------------------------------------------------------------------------
#  Main Program
# -----------------------------------------------------------------------------
def __main():
    
    
    setup_image_location = "./images/setup.jpeg"
    while True:
        user_choice = __menu_choice()
        
        if user_choice == '1':
            
            camera = imageread.setup_camera()
            camera.start_preview()
            
            print "INFO: Setup image save location:", str(setup_image_location)
            print "INFO: Camera preview initiated."
            print ""
            
            
            while True:
                raw_input("When ready, press ENTER to capture setup image.")
                camera.capture(setup_image_location)
                user_input = raw_input("Accept image (y/n)? > ")
                
                if user_input.lower() in ('y', "yes"): break
                    
            # picture saved, end preview
            camera.close()
            print ""
            print "INFO: Setup image has been saved to:", str(setup_image_location)
            print "INFO: Camera preview closed."
        
        
        elif user_choice == '2':
            
            try:
                setup_image = Image.open(setup_image_location)
            except:
                print "ERROR: Setup image does not exist. Select option 1 to"
                print "create a new setup image."
            
        
            
            raw_input("\nPress ENTER to continue...\n")
            setup_selectarea.main(setup_image)
        
        
        elif user_choice == '3':
           
            try:
                import setup_data
                boxes = setup_data.boxes
                if not isinstance(boxes, list): raise ValueError()
            except:
                print "ERROR: Setup data does not exist. Please run options 1 and 2 first."
                continue
                
            # attempt to import the server senddata module. If fail, return to main menu
            # prompt.
            try:
                import senddata
            except:
                print "ERROR: Could not import send data file."
                continue
            
            # deregister all areas associated with this pi (start fresh)
            out = senddata.deregister_pi()
            
            try:
                out['error']
                print "ERROR: Error in connecting to server. Please update settings.py."
                continue
            except:
                pass
            
            # register each box on the server
            for box in boxes:
                if box[1] == 0:
                    senddata.register_area(box[0])
                    print "INFO: Registering area", box[0], "on server database."
                    
            print "\nRegistration complete."
        
        
        elif user_choice == '4':
            print "This will complete the setup and run the main PiPark program."
            user_input = raw_input("Continue and run the main program? (y/n) >")
            if user_input.lower in ('y', 'yes'):
                main.run_main()
                break
                                                 
        elif user_choice.lower() in ('h', "help"):
        
            
            
        elif user_choice.lower() in ('q', "quit"):
            print ""
            break
        
        
        else:
            print "\nERROR: Invalid menu choice.\n"




def __menu_choice():
    
    
    user_choice = raw_input("> ")
    
    return user_choice


if __name__ == "__main__":
    __main()
