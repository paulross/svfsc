var searchData=
[
  ['pass_580',['pass',['../class_s_v_f_s_1_1_test_1_1_test_count.html#a1d525998e7c133e08af4c3f56e887cbb',1,'SVFS::Test::TestCount']]],
  ['private_5fsparsevirtualfile_5fsvf_5fread_5fas_5fpy_5fbytes_581',['private_SparseVirtualFile_svf_read_as_py_bytes',['../__c_s_v_f_8cpp.html#a05c3a042105cc16cf80d25e729395aad',1,'_cSVF.cpp']]],
  ['pydoc_5fstrvar_582',['PyDoc_STRVAR',['../__c_s_v_f_8cpp.html#af76f2ef83d2a9d4704e1e980e2a4d087',1,'PyDoc_STRVAR(cp_SparseVirtualFile_id_docstring, &quot;id(self) -&gt; str\n\n&quot; &quot;Returns the ID of the Sparse Virtual File.&quot;):&#160;_cSVF.cpp'],['../__c_s_v_f_8cpp.html#a0983ec9ed5b3d32bfa17db098f581486',1,'PyDoc_STRVAR(cp_SparseVirtualFile_has_data_docstring, &quot;has_data(self, file_position: int, length: int) -&gt; bool\n\n&quot; &quot;Checks if the Sparse Virtual File of the ID has data at the given ``file_position`` and ``length``.\n\n&quot; &quot;Parameters\n\n&quot; &quot;file_position: int\n&quot; &quot;    The absolute file position of the start of the data.\n\n&quot; &quot;length: int\n&quot; &quot;    The length of the required data in bytes.\n\n&quot; &quot;Returns\n\n&quot; &quot;bool: True if the SVF contains the data, False otherwise.&quot;):&#160;_cSVF.cpp'],['../__c_s_v_f_8cpp.html#a37067b0baf903304708173b2f6fb8ff2',1,'PyDoc_STRVAR(cp_SparseVirtualFile_write_docstring, &quot;write(self, file_position: int, data: bytes) -&gt; None\n\n&quot; &quot;Writes the data to the Sparse Virtual File of the given ID at ``file_position`` and ``data`` as a ``bytes`` object.&quot; &quot; This will raise an ``IOError`` if ``self.compare_for_diff`` is True and given data is different than&quot; &quot; that seen before and only new data up to this point will be written.&quot; &quot; If the ``byte`` data is empty nothing will be done.&quot; &quot; This will raise a RuntimeError if the data can not be written for any other reason&quot;):&#160;_cSVF.cpp'],['../__c_s_v_f_8cpp.html#ae56a02b2bcbed1e96aac28ccd1f8a3ac',1,'PyDoc_STRVAR(cp_SparseVirtualFile_read_docstring, &quot;read(self, file_position: int, length: int) -&gt; bytes\n\n&quot; &quot;Read the data from the Sparse Virtual File at ``file_position`` and ``length`` returning a ``bytes`` object.&quot; &quot; This takes a file position and a length.&quot; &quot; This will raise an ``IOError`` if any data is not present&quot; &quot; This will raise a ``RuntimeError`` if the data can not be read for any other reason&quot;):&#160;_cSVF.cpp'],['../__c_s_v_f_8cpp.html#a8cb8195fbc1ad7da04aa79dc2f4d432d',1,'PyDoc_STRVAR(cp_SparseVirtualFile_erase_docstring, &quot;erase(self, file_position: int) -&gt; None\n\n&quot; &quot;Erase the data from the Sparse Virtual File at the given ``file_position``&quot; &quot; which must be the beginning of a block.\n&quot; &quot;This will raise an ``IOError`` if a block is not present at that file position.&quot;):&#160;_cSVF.cpp'],['../__c_s_v_f_8cpp.html#a997596c4a941c554de77ac74761d32ef',1,'PyDoc_STRVAR(cp_SparseVirtualFile_need_docstring, &quot;need(self, file_position: int, length: int, greedy_length: int = 0) -&gt; typing.Tuple[typing.Tuple[int, int], ...]\n\n&quot; &quot;Given a file_position and length this returns a ordered list ``[(file_position, length), ...]`` of seek/read&quot; &quot; instructions of data that is required to be written to the Sparse Virtual File so that a subsequent read will&quot; &quot; succeed.\n&quot; &quot; If greedy_length is &gt; 0 then, if possible, blocks will be coalesced to reduce the size of the return value.&quot; &quot;\n\n&quot; &quot;.. note::\n&quot; &quot;    If a greedy_length is given this will be the *minimum* size of the length of the required block.&quot; &quot;    If the length of the required block is so large (because of existing blocks likely to be coalesced)&quot; &quot;    then the user might want to split the length, for example, into multiple (smaller) GET requests which&quot; &quot;    will then be coalesced on ``write()``&quot; &quot;\n\n&quot; &quot;.. warning::\n&quot; &quot;    The SVF has no knowledge of the the actual file size so when using a greedy length the need list might&quot; &quot; include positions beyond EOF.\n\n&quot; &quot;    For example a file 1024 bytes long and a greedy length of 256 then ``need(1000, 24, 256)`` will create&quot; &quot; a need list of ``[(1000, 256),]``.&quot; &quot; This should generate a ``write(1000, 24)`` not a ``write(1000, 256)``.\n\n&quot; &quot;    It is up to the caller to handle this, however, ``reads()`` in C/C++/Python will ignore read lengths past&quot; &quot; EOF so the caller does not have to do anything.\n\n&quot; &quot;\n\nUsage::\n\n&quot; &quot;    if not svf.has_data(position, length):\n&quot; &quot;        for read_fpos, read_length in svf.need(position, length):\n&quot; &quot;            # Somehow get data as a bytes object at read_fpos, read_length...\n&quot; &quot;            svf.write(read_fpos, data)\n&quot; &quot;    return svf.read(position, length):\n&quot;):&#160;_cSVF.cpp'],['../__c_s_v_f_8cpp.html#a08bca32f9fbfb1c32464511e0190c506',1,'PyDoc_STRVAR(cp_SparseVirtualFile_need_many_docstring, &quot;need_many(self, seek_reads: typing.List[typing.Tuple[int, int]], greedy_length: int = 0) -&gt; typing.Tuple[typing.Tuple[int, int], ...]\n\n&quot; &quot;Given a list of (file_position, length) this returns a ordered list ``[(file_position, length), ...]`` of seek/read&quot; &quot; instructions of data that is required to be written to the Sparse Virtual File so that a subsequent read will&quot; &quot; succeed.\n\n&quot; &quot;If greedy_length is &gt; 0 then, if possible, blocks will be coalesced to reduce the size of the return value.&quot; &quot;\n\n&quot; &quot;See also :py:meth:`svfsc.cSVF.need`&quot;):&#160;_cSVF.cpp'],['../__c_s_v_f_8cpp.html#a970663a099499383430ed58805962679',1,'PyDoc_STRVAR(cp_SparseVirtualFile_blocks_docstring, &quot;blocks(self) -&gt; typing.Tuple[typing.Tuple[int, int], ...]\n\n&quot; &quot;This returns a ordered tuple ``((file_position, length), ...)``&quot; &quot; of the shape of the blocks held by the SVF in file position order.&quot;):&#160;_cSVF.cpp'],['../__c_s_v_f_8cpp.html#ae17458497245c0f563a1c054e45ae08c',1,'PyDoc_STRVAR(cp_SparseVirtualFile_block_touches_docstring, &quot;block_touches(self) -&gt; typing.Dict[int, int]\n\n&quot; &quot;This returns a dict ``{touch_int: file_position, ...}``&quot; &quot; of the touch integer of each block mapped to the file position.\n&quot; &quot;The caller can decide what older blocks can be used the erase(file_position).&quot;):&#160;_cSVF.cpp'],['../__c_s_v_f_8cpp.html#aa3b2e841a243b339d34e3f4fdcb8bbdc',1,'PyDoc_STRVAR(cp_SparseVirtualFile_lru_punt_docstring, &quot;lru_punt(self, cache_size_upper_bound: int) -&gt; int\n\n&quot; &quot;Reduces the size of the cache to &lt; the given size by removing older blocks, at least one block will be left.\n&quot; &quot;There are limitations to this tactic, see the documentation in Technical Notes -&gt; Cache Punting.&quot;):&#160;_cSVF.cpp'],['../__c_s_v_f_8cpp.html#a6e17e53f782895d8fa23b16396a77ad6',1,'PyDoc_STRVAR(cp_SparseVirtualFile_file_mod_time_matches_docstring, &quot;file_mod_time_matches(self, file_mod_time: float) -&gt; bool\n\n&quot; &quot;Returns True if the file modification time of the Sparse Virtual File matches the given time as a float.&quot;):&#160;_cSVF.cpp'],['../__c_s_v_f_8cpp.html#a9bbe1c77ad9f27b3342778ec55670997',1,'PyDoc_STRVAR(cp_SparseVirtualFile_file_mod_time_docstring, &quot;file_mod_time(self) -&gt; float\n\n&quot; &quot;Returns the file modification time as a float in UNIX time of the Sparse Virtual File.&quot;):&#160;_cSVF.cpp'],['../__c_s_v_f_8cpp.html#ae5329698611ee5cce5047977a66a7349',1,'PyDoc_STRVAR(cp_SparseVirtualFile_time_write_docstring, &quot;time_write(self) -&gt; typing.Optional[datetime.datetime]\n\n&quot; &quot;Returns the timestamp of the last write to the Sparse Virtual File as a ``datetime.datetime``&quot; &quot; or ``None`` if no write has taken place.&quot;):&#160;_cSVF.cpp'],['../__c_s_v_f_8cpp.html#aacbd3b13373d04d4bae5c7bd3937d481',1,'PyDoc_STRVAR(cp_SparseVirtualFile_time_read_docstring, &quot;time_read(self) -&gt; typing.Optional[datetime.datetime]\n\n&quot; &quot;Returns the timestamp of the last read from the Sparse Virtual File as a ``datetime.datetime``&quot; &quot; or ``None`` if no read has taken place.&quot;):&#160;_cSVF.cpp'],['../__c_s_v_f_8cpp.html#a1f089d9e9354b0942662e3ec6508cb22',1,'PyDoc_STRVAR(cp_SparseVirtualFile_config_docstring, &quot;config(self) -&gt; typing.Dict[str, bool]\n\n&quot; &quot;Returns the SVF configuration as a dict.&quot;):&#160;_cSVF.cpp'],['../__c_s_v_f_8cpp.html#a64de1869514950a178f428a4db3ae9af',1,'PyDoc_STRVAR(svfs_cSVF_doc, &quot;This class implements a Sparse Virtual File (SVF).&quot; &quot; This is an in-memory file that has fragments of a real file.&quot; &quot; It has read/write operations and can describe what file fragments are needed, if any, before any read operation.&quot; &quot;\n\n&quot; &quot;The constructor takes a string as an ID and optionally:\n&quot; &quot; - A file modification time as a float (default 0.0).&quot; &quot; This can be used for checking if the actual file might been changed which might invalidate the SVF.\n&quot; &quot; - ``overwrite_on_exit``, a boolean that will overwrite the memory on destruction (default ``False``).&quot; &quot; If ``True`` then ``clear()`` on a 1Mb SVF typically takes 35 µs, if ``False`` 1.5 µs.\n&quot; &quot; - ``compare_for_diff``, a boolean that will check that overlapping writes match (default ``True``).&quot; &quot; If ``True`` this adds about 25% time to an overlapping write but gives better chance of catching changes to the&quot; &quot; original file.\n&quot; &quot;\n\n&quot; &quot;For example::&quot; &quot;\n\n&quot; &quot;       import svfsc\n&quot; &quot;       \n&quot; &quot;       svf = svfsc.cSVF(&apos;some ID&apos;)\n&quot; &quot;       svf.write(12, b&apos;ABCD&apos;)\n&quot; &quot;       svf.read(13, 2)  # Returns b&apos;BC&apos;\n&quot; &quot;       svf.need(10, 12)  # Returns ((10, 2), 16, 6)), the file positions and lengths the the SVF needs\n&quot; &quot;       svf.read(1024, 18)  # SVF raises an error as it has no data here.\n&quot; &quot;\n&quot; &quot;Signature:\n\n``svfsc.cSVF(id: str, mod_time: float = 0.0, overwrite_on_exit: bool = False, compare_for_diff: bool = True)``&quot;):&#160;_cSVF.cpp'],['../__c_s_v_f_s_8cpp.html#af25c00e43cba1c954994dc780d323720',1,'PyDoc_STRVAR(cp_SparseVirtualFileSystem_keys_docstring, &quot;keys(self) -&gt; typing.List[str]\n\n&quot; &quot;Returns the IDs of all the Sparse Virtual Files in the Sparse Virtual File System.&quot;):&#160;_cSVFS.cpp'],['../__c_s_v_f_s_8cpp.html#a370da683d567130329b4796a51023e04',1,'PyDoc_STRVAR(cp_SparseVirtualFileSystem_insert_docstring, &quot;insert(self, id: str) -&gt; None\n\n&quot; &quot;Inserts a Sparse Virtual File of ID and Unix file modification time as a float.&quot;):&#160;_cSVFS.cpp'],['../__c_s_v_f_s_8cpp.html#a4e771053c9e5495b21e9b1156716bf4e',1,'PyDoc_STRVAR(cp_SparseVirtualFileSystem_remove_docstring, &quot;remove(self, id: str) -&gt; None\n\n&quot; &quot;Removes a Sparse Virtual File of ID freeing that file&apos;s memory. Will raise an ``IndexError`` if the ID is absent.&quot;):&#160;_cSVFS.cpp'],['../__c_s_v_f_s_8cpp.html#afb4b348b2d23b680cc1376de10051665',1,'PyDoc_STRVAR(cp_SparseVirtualFileSystem_has_docstring, &quot;has(self, id: str) -&gt; bool\n\n&quot; &quot;Returns True if the Sparse Virtual File for the ID is in the Sparse Virtual File System.&quot;):&#160;_cSVFS.cpp'],['../__c_s_v_f_s_8cpp.html#ab52dab7939b48980f45d4fbc9b567566',1,'PyDoc_STRVAR(cp_SparseVirtualFileSystem_total_size_of_docstring, &quot;total_size_of(self) -&gt; int\n\n&quot; &quot;Returns the estimate of total memory usage of the Sparse Virtual File System.&quot;):&#160;_cSVFS.cpp'],['../__c_s_v_f_s_8cpp.html#a00929548a191c7c01e04ca42738566b5',1,'PyDoc_STRVAR(cp_SparseVirtualFileSystem_total_bytes_docstring, &quot;total_bytes(self) -&gt; int\n\n&quot; &quot;Returns the total number of file bytes held by the Sparse Virtual File System.&quot;):&#160;_cSVFS.cpp'],['../__c_s_v_f_s_8cpp.html#a658309cef05bd88c45b3525c19a35d36',1,'PyDoc_STRVAR(cp_SparseVirtualFileSystem_total_blocks_docstring, &quot;total_blocks(self) -&gt; int\n\n&quot; &quot;Returns the total number of blocks of data held by the Sparse Virtual File System.&quot;):&#160;_cSVFS.cpp'],['../__c_s_v_f_s_8cpp.html#abde88766688ae4ea1ce2470bccbef5fd',1,'PyDoc_STRVAR(cp_SparseVirtualFileSystem_svf_has_data_docstring, &quot;has_data(self, id: str, file_position: int, length: int) -&gt; bool\n\n&quot; &quot;Checks if the Sparse Virtual File of the ID has data at the given file_position and length.&quot; &quot; This takes a string as an id, a file position and a length.&quot; &quot; This returns True if the Sparse Virtual File of that id has the data, False otherwise.&quot; &quot; This will raise an ``IndexError`` if the SVF of that id does not exist.&quot;):&#160;_cSVFS.cpp'],['../__c_s_v_f_s_8cpp.html#a7331052188dfc741f9f65ab456e043cb',1,'PyDoc_STRVAR(cp_SparseVirtualFileSystem_svf_write_docstring, &quot;write(self, id: str, file_position: int, data: bytes) -&gt; None\n\n&quot; &quot;Writes the data to the Sparse Virtual File of the given ID at file_position and length.\n\n&quot; &quot;This takes a string as an id, a file position and data as a bytes object.\n&quot; &quot;This will raise an ``IndexError`` if the SVF of that id does not exist.\n&quot; &quot;This will raise an ``IOError`` if the given data is different than that seen before and only\n&quot; &quot;new data up to this point will be written.\n&quot; &quot;This will raise a ``RuntimeError`` if the data can not be written for any other reason.\n&quot;):&#160;_cSVFS.cpp'],['../__c_s_v_f_s_8cpp.html#a76c7fa4cf0614cc8eab8a70945848c98',1,'PyDoc_STRVAR(cp_SparseVirtualFileSystem_svf_read_docstring, &quot;read(self, id: str, file_position: int, length: int) -&gt; bytes\n\n&quot; &quot;Read the data to the Sparse Virtual File at file_position and length returning a bytes object.\n&quot; &quot;This takes a string as an id, a file position and a length.\n&quot; &quot;\nThis will raise an ``IndexError`` if the Sparse Virtual File of that id does not exist.\n&quot; &quot;This will raise an ``IOError`` if any data is not present\n&quot; &quot;This will raise a ``RuntimeError`` if the data can not be read for any other reason.\n&quot;):&#160;_cSVFS.cpp'],['../__c_s_v_f_s_8cpp.html#a6884218aa541be3f8c405d32b0737a39',1,'PyDoc_STRVAR(cp_SparseVirtualFileSystem_svf_erase_docstring, &quot;erase(self, id: str, file_position: int) -&gt; None\n\n&quot; &quot;Erases the data block in the Sparse Virtual File at a file position.\n&quot; &quot;This takes a string as an id and a file_position.\n&quot; &quot;This will raise an ``IndexError`` if the Sparse Virtual File of that id does not exist.\n&quot; &quot;This will raise an ``IOError`` if there is not a block at the position.\n&quot; &quot;This will raise a ``RuntimeError`` if the data can not be read for any other reason.\n&quot;):&#160;_cSVFS.cpp'],['../__c_s_v_f_s_8cpp.html#afea5503a6850544fb8a3f276f5243a56',1,'PyDoc_STRVAR(cp_SparseVirtualFileSystem_svf_need_docstring, &quot;need(self, id: str, file_position: int, length: int, greedy_length: int = 0) -&gt; typing.Tuple[typing.Tuple[int, int], ...]\n\n&quot; &quot;Given a file_position and length this returns a ordered list ``[(file_position, length), ...]`` of seek/read&quot; &quot; instructions of data that is required to be written to the Sparse Virtual File so that a subsequent read will succeed.\n&quot; &quot;If greedy_length is &gt; 0 then, if possible, blocks will be coalesced to reduce the size of the return value.&quot; &quot;\nUsage::\n\n&quot; &quot;    if not svfs.has(identity, file_position, length):\n&quot; &quot;        for fpos, read_len in svfs.need(identity, file_position, length):\n&quot; &quot;            # Somehow get the data at that seek/read position...\n&quot; &quot;            svfs.write(identity, fpos, data)\n&quot; &quot;    return svfs.read(identity, file_position, length):\n&quot;):&#160;_cSVFS.cpp'],['../__c_s_v_f_s_8cpp.html#aa30742a7b888881833d015229f00097c',1,'PyDoc_STRVAR(cp_SparseVirtualFileSystem_svf_need_many_docstring, &quot;need_many(self, id: str, seek_reads: typing.List[typing.Tuple[int, int]], greedy_length: int = 0) -&gt; typing.Tuple[typing.Tuple[int, int], ...]\n\n&quot; &quot;Given a list of (file_position, length) this returns a ordered list ``[(file_position, length), ...]`` of seek/read&quot; &quot; instructions of data that is required to be written to the Sparse Virtual File so that a subsequent read will&quot; &quot; succeed.\n\n&quot; &quot;If greedy_length is &gt; 0 then, if possible, blocks will be coalesced to reduce the size of the return value.&quot; &quot;\n\n&quot; &quot;See also :py:meth:`svfsc.cSVFS.need`&quot;):&#160;_cSVFS.cpp'],['../__c_s_v_f_s_8cpp.html#af4c02d1101c8e6bdf208d584e228f538',1,'PyDoc_STRVAR(cp_SparseVirtualFileSystem_svf_blocks_docstring, &quot;blocks(self, id: str) -&gt; typing.Tuple[typing.Tuple[int, int], ...]\n\n&quot; &quot;This returns a ordered tuple ((file_position, length), ...) of all the blocks held by the SVF identified&quot; &quot;by the given id.\n&quot; &quot;This will raise an ``IndexError`` if the Sparse Virtual File of that id does not exist.&quot;):&#160;_cSVFS.cpp'],['../__c_s_v_f_s_8cpp.html#a6a0f5f963b4adce221e5550f86c74091',1,'PyDoc_STRVAR(cp_SparseVirtualFileSystem_svf_block_touches_docstring, &quot;block_touches(self, id: str) -&gt; typing.Dict[int, int]\n\n&quot; &quot;This returns a dict ``{touch_int: file_position, ...}``&quot; &quot; of the touch integer of each block mapped to the file position.\n&quot; &quot;The caller can decide what older blocks can be used the erase(file_position).&quot;):&#160;_cSVFS.cpp'],['../__c_s_v_f_s_8cpp.html#ad767c904227c542b882fb195aaa196e0',1,'PyDoc_STRVAR(cp_SparseVirtualFileSystem_svf_lru_punt_docstring, &quot;lru_punt(self, id: str, cache_size_upper_bound: int) -&gt; int\n\n&quot; &quot;Reduces the size of the cache to &lt; the given size by removing older blocks, at least one block will be left.\n&quot; &quot;There are limitations to this tactic, see the documentation in Technical Notes -&gt; Cache Punting.&quot;):&#160;_cSVFS.cpp'],['../__c_s_v_f_s_8cpp.html#aa66c13f6f0e8ed4e099a0ec7432920c3',1,'PyDoc_STRVAR(cp_SparseVirtualFileSystem_svf_lru_punt_all_docstring, &quot;lru_punt_all(self, cache_size_upper_bound: int) -&gt; int\n\n&quot; &quot;Reduces the size of all IDs in the cache to &lt; the given size by removing older blocks.&quot; &quot; At least one block will be left for each ID.\n&quot; &quot;There are limitations to this tactic, see the documentation in Technical Notes -&gt; Cache Punting.&quot;):&#160;_cSVFS.cpp'],['../__c_s_v_f_s_8cpp.html#acf95b8273df72893d204ce8ec7328672',1,'PyDoc_STRVAR(cp_SparseVirtualFileSystem_svf_file_mod_time_matches_docstring, &quot;file_mod_time_matches(self, id: str) -&gt; bool\n\n&quot; &quot;Returns True if the file modification time of the Sparse Virtual File identified by the given id the matches&quot; &quot;the given time as a float.\n&quot; &quot;This will raise an ``IndexError`` if the Sparse Virtual File of that id does not exist.&quot;):&#160;_cSVFS.cpp'],['../__c_s_v_f_s_8cpp.html#ae15a5e629a531b3c7537fc4053ec4bd3',1,'PyDoc_STRVAR(cp_SparseVirtualFileSystem_file_mod_time_docstring, &quot;file_mod_time(self, id: str) -&gt; float\n\n&quot; &quot;Returns the file modification time as a float in UNIX time of the Sparse Virtual File identified by the given id.\n&quot; &quot;This will raise an ``IndexError`` if the Sparse Virtual File of that id does not exist.&quot;):&#160;_cSVFS.cpp'],['../__c_s_v_f_s_8cpp.html#ae5936fd8051e3abca94ab41a75d7d961',1,'PyDoc_STRVAR(cp_SparseVirtualFileSystem_svf_time_write_docstring, &quot;time_write(self, id: str) -&gt; typing.Optional[datetime.datetime]\n\n&quot; &quot;Returns the timestamp of the last write to the Sparse Virtual File identified by the given id as a datetime.datetime.&quot; &quot;or None if no write has taken place.\n&quot; &quot;This will raise an ``IndexError`` if the Sparse Virtual File of that id does not exist.&quot;):&#160;_cSVFS.cpp'],['../__c_s_v_f_s_8cpp.html#a84c9c1ba9140038283d340cbfebbcb14',1,'PyDoc_STRVAR(cp_SparseVirtualFileSystem_svf_time_read_docstring, &quot;time_read(self, id: str) -&gt; typing.Optional[datetime.datetime]\n\n&quot; &quot;Returns the timestamp of the last read from the Sparse Virtual File identified by the given id as a datetime.datetime.&quot; &quot;or None if no read has taken place.\n&quot; &quot;This will raise an ``IndexError`` if the Sparse Virtual File of that id does not exist.&quot;):&#160;_cSVFS.cpp'],['../__c_s_v_f_s_8cpp.html#a6227bfb1e19d35f90f9f0a817694918f',1,'PyDoc_STRVAR(cp_SparseVirtualFileSystem_config_docstring, &quot;config(self) -&gt; typing.Dict[str, bool]\n\n&quot; &quot;Returns the SVFS configuration as a dict.&quot;):&#160;_cSVFS.cpp'],['../__c_s_v_f_s_8cpp.html#af38b313c878701bbfc0ac5b208b6c9f0',1,'PyDoc_STRVAR(svfs_cSVFS_doc, &quot;This class implements a Sparse Virtual File System where Sparse Virtual Files are mapped to a key (a string).\n&quot; &quot;This can be constructed with an optional boolean overwrite flag that ensures in-memory data is overwritten&quot; &quot; on destruction of any SVF.&quot;):&#160;_cSVFS.cpp']]],
  ['pyinit_5fsvfsc_583',['PyInit_svfsc',['../c_s_v_f_s_8cpp.html#a3e75495a6f8213cfe062fadfb3890cdd',1,'cSVFS.cpp']]]
];
