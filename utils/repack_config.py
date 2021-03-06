import hashlib
import shutil
import zipfile
import json
import os

os.chdir("..")
z = zipfile.ZipFile("files/data/config_all_tr.zip", mode="w",
                    compression=zipfile.ZIP_DEFLATED)
for root, dirs, files in os.walk("config_all_tr"):
    for file in files:
        z.write(os.path.join(root, file),
                arcname=os.path.join(root,
                                     file).split("config_all_tr/")[1])
z.close()
shutil.rmtree("tmp")
with open("files/data/config_all_tr.zip", mode="rb") as f:
    hash_ = hashlib.md5(f.read()).hexdigest()
os.rename("files/data/config_all_tr.zip",
          f"files/data/config_all_tr_{hash_}.zip")
with open("files/versions.json") as f:
    versions = json.load(f)
versions["data/config_all_tr.zip"] = hash_
with open("files/versions.json", "w") as f:
    f.write(json.dumps(versions))
print(f"config_all_tr_{hash_}.zip")
