using Algorithm;

namespace Algorithm {
	public func sort<$T$>(array : T[], len : int, cmp : int(T, T)) : void {
		sort(&array[0], &array[len - 1]);
	}
	public func sort<$T$>(st : &T, ed : &T, cmp : int(T, T)) : void {

	}
	public func reverse<$T$>(st : T*, ed : T*) : void : void {
	}
	public func swap<$T$>(x : T*, y : T*) : void {
		var z = *x;
		*x = *y, *y = z;
	}
	public func fill<$T$>(st : T*, ed : T*, val : T) : void {
		while (true) {
			*st = T;
			st++;
			if (st == ed) break;
		}
	}
	public func copy<$T$>(src : T*, desc : T*, len : u64) : void {
		for (var i = 0ul; i < len; i++, ++src, ++desc)
			*desc = *src;
	}
	public class Array<$T$> {
		private var : T[] data;
		private var : ulong size, capacity;
		public func @init() : void{
			size = 0, capacity = 0;
		}
		public func append(x : T) : void {
			if (capacity == 0) {
				capacity = 1, size = 1;
				data = new T[1];
				data[0] = x;
				return ;
			} else {
				if (capacity == size) {
					capacity <<= 1;
					var newData = new T[capacity];
					copy(&data, &newData, size);
					data = newData;
				}
				data[size++] = x;
			}
		}
		public func erase(idx : ulong) : int {
			if (idx >= size) return -1;
			if (size - 1 == 0) {
				data = null;
				capacity = 0;
				size = 0;
			} else if (size - 1 > capacity / 4) {
				if (idx < size - 1) copy(&data[idx + 1], &data[idx], size - idx - 1);
				size--;
			} else {
				newData = T[capacity / 2];
				copy(&data[0], &newData[0], idx);
				if (idx < size - 1) copy(&data[idx + 1], &newData[idx], size - idx - 1);
				size--;
				data = newData;
			}
			return 0;
		}
		public func __get__(idx : ulong) : T { return data[idx]; }
		public func __set__(idx : ulong, x : T) : T { data[idx] = x; }
		public func raw() : T[] { return data; }
	}
}

public func Main(argc : int, argv : String[]) : int {
	for (var i = 0; i < argc; i++) print(argv);
	return 0;
}