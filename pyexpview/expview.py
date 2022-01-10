import numpy as np
import toml
import os.path

# Utility function for saving plots in a format ExpView can read (as a expview.toml file)
def save(directory, title, plots):
    names = []
    plot_coords = []

    for p in plots:
        names.append(p[0])

        zipped = zip(list(p[1]), list(p[2]))

        ls = [ list(coord) for coord in zipped ]

        plot_coords.append(ls)

    d = { "title": title, "names": names, "plots": plot_coords }

    with open(os.path.join(directory, "expview.toml"), "w") as f:
        f.write(toml.dumps(d))


