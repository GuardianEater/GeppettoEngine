def EpicPrint():
  print('Hello, Epic World!')

counter = 0


def update():
  global counter
  counter += 1

def late_update():
  print(counter)

def on_start():
  print("Hello im starting now")