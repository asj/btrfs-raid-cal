#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

#define u64 unsigned long long

#define FIX 0
#define LOG1 0

struct btrfs_raid_attr {
	int sub_stripes;	/* sub_stripes info for map */
	int dev_stripes;	/* stripes per dev */
	int devs_max;		/* max devs to use */
	int devs_min;		/* min devs needed */
	int tolerated_failures; /* max tolerated fail devs */
	int devs_increment;	/* ndevs has to be a multiple of this */
	int ncopies;		/* how many copies to data has */
};

enum btrfs_raid_types {
        BTRFS_RAID_RAID10,
        BTRFS_RAID_RAID1,
        BTRFS_RAID_DUP,
        BTRFS_RAID_RAID0,
        BTRFS_RAID_SINGLE,
        BTRFS_RAID_RAID5,
        BTRFS_RAID_RAID6,
        BTRFS_NR_RAID_TYPES
};

const struct btrfs_raid_attr btrfs_raid_array[BTRFS_NR_RAID_TYPES] = {
	[BTRFS_RAID_RAID10] = {
		.sub_stripes	= 2,
		.dev_stripes	= 1,
		.devs_max	= 0,	/* 0 == as many as possible */
#if FIX
		.devs_min	= 3,
#else
		.devs_min	= 4,
#endif
		.tolerated_failures = 1,
		.devs_increment	= 2,
		.ncopies	= 2,
	},
	[BTRFS_RAID_RAID1] = {
		.sub_stripes	= 1,
		.dev_stripes	= 1,
		.devs_max	= 2,
#if FIX 
		.devs_min	= 1,
#else
		.devs_min	= 2,
#endif
		.tolerated_failures = 1,
		.devs_increment	= 2,
		.ncopies	= 2,
	},
	[BTRFS_RAID_DUP] = {
		.sub_stripes	= 1,
		.dev_stripes	= 2,
		.devs_max	= 1,
		.devs_min	= 1,
		.tolerated_failures = 0,
		.devs_increment	= 1,
		.ncopies	= 2,
	},
	[BTRFS_RAID_RAID0] = {
		.sub_stripes	= 1,
		.dev_stripes	= 1,
		.devs_max	= 0,
		.devs_min	= 2,
		.tolerated_failures = 0,
		.devs_increment	= 1,
		.ncopies	= 1,
	},
	[BTRFS_RAID_SINGLE] = {
		.sub_stripes	= 1,
		.dev_stripes	= 1,
		.devs_max	= 1,
		.devs_min	= 1,
		.tolerated_failures = 0,
		.devs_increment	= 1,
		.ncopies	= 1,
	},
	[BTRFS_RAID_RAID5] = {
		.sub_stripes	= 1,
		.dev_stripes	= 1,
		.devs_max	= 0,
		.devs_min	= 2,
		.tolerated_failures = 1,
		.devs_increment	= 1,
		.ncopies	= 2,
	},
	[BTRFS_RAID_RAID6] = {
		.sub_stripes	= 1,
		.dev_stripes	= 1,
		.devs_max	= 0,
		.devs_min	= 3,
		.tolerated_failures = 2,
		.devs_increment	= 1,
		.ncopies	= 3,
	},
};

static const char *btrfs_raid_type_names[BTRFS_NR_RAID_TYPES] = {
        [BTRFS_RAID_RAID10]     = "raid10",
        [BTRFS_RAID_RAID1]      = "raid1",
        [BTRFS_RAID_DUP]        = "dup",
        [BTRFS_RAID_RAID0]      = "raid0",
        [BTRFS_RAID_SINGLE]     = "single",
        [BTRFS_RAID_RAID5]      = "raid5",
        [BTRFS_RAID_RAID6]      = "raid6",
};


void __btrfs_alloc_chunk(int index, int ndevs)
{
	int i, j, s;
	int sub_stripes    = btrfs_raid_array[index].sub_stripes;
	int dev_stripes    = btrfs_raid_array[index].dev_stripes;
	int devs_max       = btrfs_raid_array[index].devs_max;
	int devs_min       = btrfs_raid_array[index].devs_min;
	int devs_increment = btrfs_raid_array[index].devs_increment;
	int ncopies        = btrfs_raid_array[index].ncopies;
	int tolerated_failures = btrfs_raid_array[index].tolerated_failures;
	u64 num_bytes;
	u64 stripe_size;
	int data_stripes;
	int devs_stripes;
	int num_stripes;
	int missing_mirror_dev = 0;
	int x;
	int _ndevs = ndevs;
#if 0
#if LOG1
	printf("\nASJ \"%s\" ndevs %d: sub_stripes %d dev_stripes %d devs_max %d "\
		"devs_min %d tolerated_failures %d devs_increment %d ncopies %d\n",
		btrfs_raid_type_names[index], ndevs, sub_stripes,
		dev_stripes, devs_max, devs_min, tolerated_failures,
		devs_increment, ncopies);
#else
	printf("\n\"%s\" ndevs %d: sub_stripes %d dev_stripes %d devs_max %d "\
		"devs_min %d tolerated_failures %d devs_increment %d ncopies %d\n",
		btrfs_raid_type_names[index], ndevs, sub_stripes,
		dev_stripes, devs_max, devs_min, tolerated_failures,
		devs_increment, ncopies);
#endif
#endif

#if FIX
	if (ndevs > (devs_increment * sub_stripes)) {
		ndevs -= ndevs % devs_increment;
	}
#else
	ndevs -= ndevs % devs_increment;
#endif

 	if (ndevs < devs_min) {
		printf("RAID FAILED1: ndevs %d < devs_min %d\n",
				ndevs, devs_min);
	}

	if (ndevs < (devs_increment * sub_stripes)) {
#if FIX
		missing_mirror_dev = (devs_increment * sub_stripes) - ndevs;
#else
		printf("RAID FAILED2: ndevs %d < devs_increment * sub_stripes %d\n",
				ndevs, devs_increment * sub_stripes);
#endif
	}

        if (devs_max && ndevs > devs_max)
		ndevs = devs_max;

	stripe_size = 1;
	num_stripes = ndevs * dev_stripes;
	data_stripes = num_stripes / ncopies;

	switch(index) {
	case BTRFS_RAID_RAID1:
		if (missing_mirror_dev)
			data_stripes = 1;
		break;
	case BTRFS_RAID_RAID5:
		data_stripes = num_stripes - 1;
		break;
	case BTRFS_RAID_RAID6:
		data_stripes = num_stripes - 2;
		break;
	default:
		break;
	}

	for (i = 0; i < ndevs; ++i) {
		for (j = 0; j < dev_stripes; ++j) {
			int s = i * dev_stripes + j;
#if LOG1
			printf("dev %d dev_stripes %d s %d (phy_offset = dev_stripes x stripe_size) %llu\n",
				i, j, s, j * stripe_size);
#endif
		}
	}

	num_bytes = stripe_size * data_stripes;

	printf("%s\t_ndevs %d: ndevs %d num_stripes %d data_stripes %d num_bytes %d missing_mirror_dev %d\n",
		btrfs_raid_type_names[index], _ndevs,
		ndevs, num_stripes, data_stripes, num_bytes, missing_mirror_dev);
}

int main()
{
	int i;
	int ndevs_raid1 = 2;
	int ndevs_raid1_min = 1;

	int ndevs_raid10 = 4;
	int ndevs_raid10_min = 3;

	int ndevs_raid5 = 3;
	int ndevs_raid5_min = 2;

#if 1
	__btrfs_alloc_chunk(BTRFS_RAID_RAID1, ndevs_raid1_min);
	__btrfs_alloc_chunk(BTRFS_RAID_RAID1, ndevs_raid1);
	__btrfs_alloc_chunk(BTRFS_RAID_RAID1, 3);
	__btrfs_alloc_chunk(BTRFS_RAID_RAID1, 4);
	__btrfs_alloc_chunk(BTRFS_RAID_RAID1, 5);
	__btrfs_alloc_chunk(BTRFS_RAID_RAID1, 6);
	__btrfs_alloc_chunk(BTRFS_RAID_RAID1, 7);

printf("\n");
	__btrfs_alloc_chunk(BTRFS_RAID_RAID10, ndevs_raid10_min);
	__btrfs_alloc_chunk(BTRFS_RAID_RAID10, ndevs_raid10);
	__btrfs_alloc_chunk(BTRFS_RAID_RAID10, 5);
	__btrfs_alloc_chunk(BTRFS_RAID_RAID10, 6);
	__btrfs_alloc_chunk(BTRFS_RAID_RAID10, 7);

printf("\n");
	__btrfs_alloc_chunk(BTRFS_RAID_RAID5, ndevs_raid5_min);
	__btrfs_alloc_chunk(BTRFS_RAID_RAID5, ndevs_raid5);
	__btrfs_alloc_chunk(BTRFS_RAID_RAID5, 4);
	__btrfs_alloc_chunk(BTRFS_RAID_RAID5, 5);
	__btrfs_alloc_chunk(BTRFS_RAID_RAID5, 6);
	__btrfs_alloc_chunk(BTRFS_RAID_RAID5, 7);

	__btrfs_alloc_chunk(BTRFS_RAID_DUP, 1);
	__btrfs_alloc_chunk(BTRFS_RAID_RAID0, 1);
	__btrfs_alloc_chunk(BTRFS_RAID_SINGLE, 1);

#else
	for (i = 0; i < BTRFS_NR_RAID_TYPES; i++) {
		__btrfs_alloc_chunk(i, 1);
	}
#endif
	return 0;
}
