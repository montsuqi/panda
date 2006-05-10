class FileUp
  def start(node, db)
    puts("File")
    fileup = node.windows["file"]
    fileup.filename1 = ""
    fileup.body1 = 0
    node.put_window
  end
  def do_down(node, db)
    fileup = node.windows["file"]
    fileup.filename2 = "down.1"
    fileup.body2 = 2
    node.put_window
  end
  def do_up(node, db)
    fileup = node.windows["file"]
    blob = db["blob"]
    blob.file = fileup.filename1
    blob.object = fileup.body1
    blob.exec("BLOBEXPORT")
    puts(fileup.filename1)
    node.put_window
  end
end
