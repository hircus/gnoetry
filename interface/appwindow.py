
import gnoetics

import gtk
import red_menubar, red_toolbar
import about

from poemtileview import *
from poemtextview import *

class AppWindow(gtk.Window):

    __total_app_window_count = 0

    def __init__(self):
        gtk.Window.__init__(self)
        self.set_title("Gnoetry")

        AppWindow.__total_app_window_count += 1
        self.connect("delete_event", lambda aw, ev: aw.__close_window())

        self.__vbox = gtk.VBox(0, 0)
        self.add(self.__vbox)

        self.__accel_group = gtk.AccelGroup()
        self.add_accel_group(self.__accel_group)

        ### Menubar
        self.__menubar = red_menubar.MenuBar(self.__accel_group)
        self.__menubar.set_user_data(self)
        self.__assemble_menubar(self.__menubar)
        self.__vbox.pack_start(self.__menubar, expand=0, fill=1)

        ### Toolbar
        #self.__toolbar = red_toolbar.Toolbar()
        #self.__assemble_toolbar(self.__toolbar)
        #self.__vbox.pack_start(self.__toolbar, expand=0, fill=1)


        ### The poem views
        self.__tileview = PoemTileView()
        self.__textview = PoemTextView()

        vpaned = gtk.VPaned()

        sw = gtk.ScrolledWindow()
        sw.set_policy(gtk.POLICY_AUTOMATIC, gtk.POLICY_AUTOMATIC)
        sw.add_with_viewport(self.__tileview)

        vpaned.pack1(sw, 1, 1)
       
        sw = gtk.ScrolledWindow()
        sw.set_policy(gtk.POLICY_AUTOMATIC, gtk.POLICY_AUTOMATIC)
        sw.add_with_viewport(self.__textview)

        vpaned.pack2(sw, 0, 1)

        self.__vbox.pack_start(vpaned, expand=1, fill=1)

        ### Statusbar
        self.__statusbar = gtk.Statusbar()
        self.__statusbar.show()
        self.__vbox.pack_start(self.__statusbar, expand=0, fill=1)
        self.__menubar.set_statusbar(self.__statusbar)

        self.__vbox.show_all()


    def set_poem(self, p):
        self.__tileview.set_poem(p)
        self.__textview.set_poem(p)
        

    def __assemble_menubar(self, bar):
        bar.add("/_File")
        bar.add("/_Edit")
        bar.add("/_Help")

        bar.add("/_File/_New",
                stock=gtk.STOCK_NEW,
                description="Open a new Gnoetry window")
        bar.add("/_File/_Save",
                stock=gtk.STOCK_SAVE,
                description="Save the current poem to a text file")
        bar.add("/_File/_Print",
                stock=gtk.STOCK_PRINT,
                description="Print the current poem")
        bar.add("/_File/sep", is_separator=1)
        bar.add("/_File/_Close",
                stock=gtk.STOCK_CLOSE,
                description="Close this window")
        bar.add("/_File/_Quit",
                stock=gtk.STOCK_QUIT,
                description="Close all windows and exit Gnoetry")

        bar.add("/_Edit/Clear",
                stock=gtk.STOCK_CLEAR,
                description="Clear this poem")

        bar.add("/_Help/About Gnoetry",
                description="Learn more about Gnoetry",
                callback=lambda x: about.show_about_gnoetry())
        bar.add("/_Help/About Beard of Bees",
                description="Learn more about Beard of Bees",
                callback=lambda x: about.show_about_beard_of_bees())
        bar.add("/_Help/About Ubu Roi",
                description="Learn more about Ubu Roi",
                callback=lambda x: about.show_about_ubu_roi())


    def __assemble_toolbar(self, bar):
        pass


    def __new_window(self):
        new_win = AppWindow()
        new_win.show_all()
        

    def __close_window(self):
        self.destroy()
        AppWindow.__total_app_window_count -= 1
        if AppWindow.__total_app_window_count == 0:
            gtk.mainquit()
        return gtk.TRUE
            
