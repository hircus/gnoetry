
import gobject, gtk, gnoetics
import sys, os, threading

class WeightPicker(gtk.Dialog):

    def __init__(self, weights,
                 callback=None,
                 callback_args=()):
        gtk.Dialog.__init__(self)

        self.__callback = callback
        self.__callback_args = callback_args

        def response_cb(w, code):
            if self.__callback:
                self.__callback(w, *self.__callback_args)
        self.connect("response", response_cb)

        def delete_event_cb(w, ev):
            if self.__callback:
                self.__callback(w, *self.__callback_args)
            return True
        self.connect("delete_event", delete_event_cb)

        self.set_title("Weight Your Source Texts")

        self.__weights = weights

        self.__button_ok = self.add_button(gtk.STOCK_OK,
                                           gtk.RESPONSE_CLOSE)
        
        all_texts = self.__weights.keys()
        all_texts.sort(lambda x, y: cmp(x.get_sort_title(),
                                        y.get_sort_title()))

        table = gtk.Table(len(all_texts), 3, False)

        self.__labels = []

        for i, txt in enumerate(all_texts):
            label = gtk.Label(txt.get_title())
            table.attach(label, 0, 1, i, i+1,
                         gtk.FILL, 0,
                         4, 3)

            adj = gtk.Adjustment(weights[txt], 0.0, 10, 0.5, 1, 0)
            scale = gtk.HScale(adj)
            scale.set_draw_value(False)
            scale.set_size_request(100, -1)
            table.attach(scale, 1, 2, i, i+1,
                         gtk.EXPAND | gtk.FILL, 0,
                         4, 3)

            wt_label = gtk.Label("")
            table.attach(wt_label, 2, 3, i, i+1,
                         gtk.FILL, 0,
                         4, 3)

            self.__labels.append((txt, wt_label))
            self.__update_labels()

            def value_changed_cb(adj, txt, picker):
                x = int(adj.get_value()*10+0.5)/10.0
                picker.set_weight(txt, x)
                picker.__update_labels()
            
            adj.connect("value_changed", value_changed_cb, txt, self)

        table.show_all()

        swin = gtk.ScrolledWindow()
        swin.set_policy(gtk.POLICY_NEVER, gtk.POLICY_AUTOMATIC)
        swin.add_with_viewport(table)
        swin.show_all()

        swin.set_size_request(-1, 200)

        self.vbox.pack_start(swin, expand=1, fill=1)

    def __update_labels(self):
        total = 0.0
        for x in self.__weights.values():
            total += x
            
        for txt, label in self.__labels:
            x = self.__weights[txt]
            label.set_text("%.1f  %4.1f%%" % (x, 100*x/total))

    def set_weight(self, txt, wt):
        self.__weights[txt] = wt

