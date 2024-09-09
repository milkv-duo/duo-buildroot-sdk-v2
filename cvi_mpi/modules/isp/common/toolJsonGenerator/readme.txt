1.architecture

  device.json     ----> device      ---
  cvi_comm_3a.h   -                     \
                   \                     \
  level.json      - --> enum,struct --------> hFile2json.py --> json Content -> tool.json --> pqtool create ui layout
                   /                     /
  cvi_comm_isp.h  -                     /
  layout.json     ----> layout      ---
  rpc.json        ----> rpc         ---

2.depend on

python script depend on re & Jinja2

3.how to install Jinja2

python -m pip install --upgrade pip
pip install Jinja2

4.instructions

note:cv183x not support
for example:
./generate_toolJson.sh cv182x
./generate_toolJson.sh cv181x
